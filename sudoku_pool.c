#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "timer.h"
#include "auxiliar.h"

// Define o tamanho do grid
#define SIZE 16
#define SUBGRID_SIZE 4
#define NUM_WORKERS 4  // Número de threads no pool
#define QUEUE_SIZE 128  // Tamanho do buffer circular de jobs

// Estrutura que representa um estado do tabuleiro para processamento
typedef struct {
    int **grid;
    int empty_cells;  // número de células vazias
    int row, col;     // última célula preenchida
    int depth;        // profundidade na árvore de busca
} SudokuState;

// Estrutura da fila circular de jobs
typedef struct {
    SudokuState *jobs[QUEUE_SIZE];
    int front, rear;
    int count;
    int active_workers;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} JobQueue;

// Variáveis globais compartilhadas
pthread_mutex_t solution_mutex;
short int solution_found = 0;
int **solution_grid;
JobQueue job_queue;

// Inicializa um novo estado do Sudoku
SudokuState* create_sudoku_state(int **grid, int size) {
    SudokuState *state = malloc(sizeof(SudokuState));
    state->grid = malloc(size * sizeof(int*));
    state->empty_cells = 0;
    
    for (int i = 0; i < size; i++) {
        state->grid[i] = malloc(size * sizeof(int));
        for (int j = 0; j < size; j++) {
            state->grid[i][j] = grid[i][j];
            if (grid[i][j] == 0) state->empty_cells++;
        }
    }
    
    state->row = state->col = 0;
    state->depth = 0;
    return state;
}

// Libera a memória de um estado
void free_sudoku_state(SudokuState *state) {
    if (!state) return;
    if (state->grid) {
        for (int i = 0; i < SIZE; i++) {
            free(state->grid[i]);
        }
        free(state->grid);
    }
    free(state);
}

// Inicializa a fila de jobs
void init_job_queue() {
    job_queue.front = job_queue.rear = job_queue.count = 0;
    job_queue.active_workers = 0;
    pthread_mutex_init(&job_queue.mutex, NULL);
    pthread_cond_init(&job_queue.not_empty, NULL);
    pthread_cond_init(&job_queue.not_full, NULL);
}

// Adiciona um job à fila
int enqueue_job(SudokuState *state) {
    pthread_mutex_lock(&job_queue.mutex);
    
    while (job_queue.count == QUEUE_SIZE && !solution_found) {
        pthread_cond_wait(&job_queue.not_full, &job_queue.mutex);
    }
    
    if (solution_found) {
        pthread_mutex_unlock(&job_queue.mutex);
        return 0;
    }
    
    job_queue.jobs[job_queue.rear] = state;
    job_queue.rear = (job_queue.rear + 1) % QUEUE_SIZE;
    job_queue.count++;
    
    pthread_cond_signal(&job_queue.not_empty);
    pthread_mutex_unlock(&job_queue.mutex);
    return 1;
}

// Remove e retorna um job da fila
SudokuState* dequeue_job() {
    pthread_mutex_lock(&job_queue.mutex);
    
    while (job_queue.count == 0 && !solution_found) {
        pthread_cond_wait(&job_queue.not_empty, &job_queue.mutex);
    }
    
    if (solution_found || job_queue.count == 0) {
        pthread_mutex_unlock(&job_queue.mutex);
        return NULL;
    }
    
    SudokuState *state = job_queue.jobs[job_queue.front];
    job_queue.front = (job_queue.front + 1) % QUEUE_SIZE;
    job_queue.count--;
    
    pthread_cond_signal(&job_queue.not_full);
    pthread_mutex_unlock(&job_queue.mutex);
    return state;
}

// Verifica se vale a pena criar um novo job a partir do estado atual
int should_split_work(SudokuState *state) {
    return state->empty_cells > 40 && state->depth < 3 && job_queue.count < QUEUE_SIZE/2;
}

// Função que processa um estado do Sudoku
void process_state(SudokuState *state) {
    int row, col;
    
    if (!find_empty_cell(state->grid, SIZE, SUBGRID_SIZE, &row, &col)) {
        pthread_mutex_lock(&solution_mutex);
        if (!solution_found) {
            solution_found = 1;
            for (int r = 0; r < SIZE; r++) {
                memcpy(solution_grid[r], state->grid[r], SIZE * sizeof(int));
            }
        }
        pthread_mutex_unlock(&solution_mutex);
        return;
    }

    // Tenta dividir o trabalho quando apropriado. Mesmo se dividir, continua
    // processando localmente para não perder alternativas.
    if (should_split_work(state)) {
        int split_count = 0;

        for (int num = 1; num <= SIZE && split_count < 4; num++) {
            if (is_valid(state->grid, SIZE, SUBGRID_SIZE, row, col, num)) {
                // Cria um novo estado para a fila
                SudokuState *new_state = create_sudoku_state(state->grid, SIZE);
                new_state->grid[row][col] = num;
                new_state->empty_cells = state->empty_cells - 1;
                new_state->depth = state->depth + 1;

                if (!enqueue_job(new_state)) {
                    free_sudoku_state(new_state);
                    return;
                }
                split_count++;
            }
        }

        // Após gerar alguns jobs, continua processando localmente para garantir
        // que todas as possibilidades sejam exploradas (não perder estados).
        if (split_count > 0) {
            for (int num = 1; num <= SIZE; num++) {
                if (is_valid(state->grid, SIZE, SUBGRID_SIZE, row, col, num)) {
                    state->grid[row][col] = num;
                    process_state(state);
                    if (solution_found) return;
                    state->grid[row][col] = 0;
                }
            }
            return;
        }
    }

    // Se não dividiu o trabalho, processa localmente usando backtracking tradicional
    for (int num = 1; num <= SIZE; num++) {
        if (is_valid(state->grid, SIZE, SUBGRID_SIZE, row, col, num)) {
            state->grid[row][col] = num;
            process_state(state);
            if (solution_found) return;
            state->grid[row][col] = 0;
        }
    }
}

// Função executada por cada worker thread
void* worker_thread(void *arg) {
    (void)arg;
    while (1) {
        pthread_mutex_lock(&job_queue.mutex);

        // Espera por um trabalho ou pela condição de término
        while (job_queue.count == 0 && !solution_found) {
            // Se não há jobs e não há workers ativos, não há mais trabalho
            if (job_queue.count == 0 && job_queue.active_workers == 0) {
                pthread_mutex_unlock(&job_queue.mutex);
                return NULL;
            }
            pthread_cond_wait(&job_queue.not_empty, &job_queue.mutex);
        }

        if (solution_found) {
            pthread_mutex_unlock(&job_queue.mutex);
            return NULL;
        }

        // Retira job da fila
        SudokuState *state = job_queue.jobs[job_queue.front];
        job_queue.front = (job_queue.front + 1) % QUEUE_SIZE;
        job_queue.count--;
        job_queue.active_workers++;

        pthread_cond_signal(&job_queue.not_full);
        pthread_mutex_unlock(&job_queue.mutex);

        if (!state) continue;

        process_state(state);
        free_sudoku_state(state);

        pthread_mutex_lock(&job_queue.mutex);
        job_queue.active_workers--;
        // Se após processar não há jobs e nenhum worker ativo, acorda todos para terminarem
        if (job_queue.count == 0 && job_queue.active_workers == 0) {
            pthread_cond_broadcast(&job_queue.not_empty);
        }
        pthread_mutex_unlock(&job_queue.mutex);

        if (solution_found) return NULL;
    }
    return NULL;
}

int main() {
    double start, end, delta;

    // Aloca memória para o grid de solução global
    solution_grid = malloc(SIZE * sizeof(int*));
    for (int i = 0; i < SIZE; i++) {
        solution_grid[i] = malloc(SIZE * sizeof(int));
    }

    // Carrega casos de teste
    TestCase* all_tests = NULL;
    int num_tests = load_test_cases(&all_tests, SIZE);
    if (num_tests == 0) {
        printf("Nenhum caso de teste foi carregado.\n");
        return 1;
    }

    printf("Sucesso! %d casos de teste carregados da memória.\n", num_tests);
    printf("--- Solucionador de Sudoku com Pool de Threads (Sudokus %dx%d) ---\n\n", SIZE, SIZE);

    // Inicializa mutexes e fila
    pthread_mutex_init(&solution_mutex, NULL);
    init_job_queue();
    
    // Estatísticas
    int successful_solutions = 0;
    double total_time = 0;
    
    // Processa cada caso de teste
    for (int t = 0; t < num_tests; t++) {
        printf("--- Exibindo Teste de Índice %d (Pool) ---\n", t + 1);
        printf("Dificuldade: %s\n", get_difficulty_label(all_tests[t].difficulty));
        
        // Reseta variáveis globais
        solution_found = 0;
        job_queue.front = job_queue.rear = job_queue.count = 0;
        job_queue.active_workers = 0;
        
        // Cria estado inicial
        SudokuState *initial_state = create_sudoku_state(all_tests[t].puzzle, SIZE);
        GET_TIME(start);
        
        // Cria worker threads
        pthread_t workers[NUM_WORKERS];
        enqueue_job(initial_state);
        
        for (int i = 0; i < NUM_WORKERS; i++) {
            pthread_create(&workers[i], NULL, worker_thread, NULL);
        }
        
        // Espera todas as threads terminarem
        for (int i = 0; i < NUM_WORKERS; i++) {
            pthread_join(workers[i], NULL);
        }
        
        GET_TIME(end);
        delta = end - start;
        total_time += delta;
        
        // Verifica se a solução está correta
        if (is_correct(SIZE, solution_grid, all_tests[t].solution)) {
            printf("Solucao correta!\n");
            successful_solutions++;
        } else {
            printf("Solucao incorreta!\n");
        }
        printf("\n");
    }
    
    // Imprime estatísticas finais
    printf("--- Estatísticas de Desempenho ---\n");
    printf("Solucoes corretas: %d de %d\n", successful_solutions, num_tests);
    printf("Tempo total: %f segundos\n", total_time);
    printf("Tempo médio: %f segundos\n", total_time / num_tests);
    
    // Libera memória
    deallocate_test_cases_and_solution(SIZE, solution_grid, all_tests, num_tests);
    pthread_mutex_destroy(&solution_mutex);
    pthread_mutex_destroy(&job_queue.mutex);
    pthread_cond_destroy(&job_queue.not_empty);
    pthread_cond_destroy(&job_queue.not_full);
    
    return 0;
}