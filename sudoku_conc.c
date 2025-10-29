#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "timer.h"
#include "aux.h"
#include <time.h>

// Define o tamanho do grid
#define SIZE 9
#define SUBGRID_SIZE 3
//#define LOG_RESULTS   //Comentar para desativar o log dos resultados detalhados

//Structs e Variáveis Globais

typedef struct {   //Struct para passar dados para cada thread. Cada thread precisa de sua cópia do grid.
    int **grid, size, size_sub_grid;
} ThreadData;

//Variáveis globais compartilhadas
pthread_mutex_t solution_mutex;   //Mutex para exclusão mútua
short int solution_found = 0;   //Flag: 1 se a solução foi encontrada
int **solution_grid;         //Matriz onde a solução final será armazenada


time_t segundos_desde_epoca;
struct tm *tempo_local;


/*
Solucionador Sequencial (Backtracking). Esta é a função que resolve o Sudoku de forma sequencial, e será chamada por 
cada thread em sua própria cópia do grid
 */
int solve_sudoku(int **grid, int size, int size_sub_grid) {
    int row, col;

    // Antes de fazer qualquer coisa, verificamos se outra thread já encontrou a solução
    pthread_mutex_lock(&solution_mutex);
    short int found = solution_found;
    pthread_mutex_unlock(&solution_mutex);

    if (found) {
        return 0;     //Para este ramo da recursão, já que alguém já ganhou
    }

    if (!find_empty_cell(grid, size, &row, &col)) {
        return 1;   //Se não há células vazias, o Sudoku está resolvido (sucesso)
    }

    //Tenta números de 1 a SIZE
    for (int num = 1; num <= SIZE; num++) {
        if (is_valid(grid, size, size_sub_grid, row, col, num)) {   //Se a posição é válida para o número
            grid[row][col] = num;

            if (solve_sudoku(grid, size, size_sub_grid)) {   //Chama recursivamente
                return 1;
            }

            grid[row][col] = 0;   //Falhou, desfaz a tentativa (backtrack)
        }
    }

    return 0;   //Nenhuma solução encontrada neste ramo
}


/*
Função que as threads executam ao serem criadas para resolver o Sudoku de forma concorrente
 */
void* solver_thread_func(void* arg) {
    ThreadData* data = (ThreadData*) arg;   //Recebe os dados (sua cópia do grid)

    //Tenta resolver o Sudoku sequencialmente a partir deste ponto
    if (solve_sudoku(data->grid, data->size, data->size_sub_grid)) {

        //Se tiver sucesso, tenta registrar como a solução global
        pthread_mutex_lock(&solution_mutex);
        
        if (!solution_found) {
            solution_found = 1;
            for (int r = 0; r < data->size; r++) {  //Copia a solução local para a global
                memcpy(solution_grid[r], data->grid[r], data->size * sizeof(int));
            }
        }

        pthread_mutex_unlock(&solution_mutex);
    }

/*
    // Obter o tempo atual em segundos desde a Época
    time(&segundos_desde_epoca);
    //printf("Tempo em segundos: %ld\n", (long)segundos_desde_epoca);

    // Converter para uma estrutura local (struct tm)
    tempo_local = localtime(&segundos_desde_epoca);

    // Imprimir os componentes da data e hora
    printf("Data e hora atuais: %02d/%02d/%d %02d:%02d:%02d\n",
           tempo_local->tm_mday,
           tempo_local->tm_mon + 1, // tm_mon vai de 0 a 11
           tempo_local->tm_year + 1900, // tm_year é anos desde 1900
           tempo_local->tm_hour,
           tempo_local->tm_min,
           tempo_local->tm_sec
           );
*/


    //Liberando a memória alocada para esta thread
    if (data->grid) {
        for (int r = 0; r < data->size; r++) free(data->grid[r]);
        free(data->grid);
    }
    free(data);
    
    pthread_exit(NULL);
}


//Função main
int main() {
    double start, end, delta;   //Variáveis para controle de tempo
    double acceleration_factor = 0.0, efficiency = 0.0;

    int **immutable_puzzle_grid = (int **) malloc(SIZE * sizeof(int *));   //É necessária uma cópia imutável do grid inicial para utilizar o is_valid
    if (immutable_puzzle_grid == NULL) { return 1; }
    for (int i = 0; i < SIZE; i++) {
        immutable_puzzle_grid[i] = (int *) malloc(SIZE * sizeof(int));
        if (immutable_puzzle_grid[i] == NULL) { 
            for (int k = 0; k < i; k++) free(immutable_puzzle_grid[k]);
            free(immutable_puzzle_grid);
            return 1; 
        }
    }

    //Aloca memória para o grid de solução global
    solution_grid = (int **) malloc(SIZE * sizeof(int *));
    if (solution_grid == NULL) {
        printf("Erro na alocação de memória para a solução\n");
        return 1;
    }
    for (int i = 0; i < SIZE; i++) {
        solution_grid[i] = (int *) malloc(SIZE * sizeof(int));
        if (solution_grid[i] == NULL) {
            printf("Erro na alocação de memória para a solução\n");
            for (int k = 0; k < i; k++) free(solution_grid[k]);
            free(solution_grid);
            return 1;
        }
    }

    TestCase* all_tests = NULL;
    int num_tests = load_test_cases(&all_tests, SIZE);

    if (num_tests == 0) {
        printf("Nenhum caso de teste foi carregado.\n");
        return 1;
    }

    printf("Sucesso! %d casos de teste carregados da memória.\n", num_tests);
    printf("--- Solucionador de Sudoku CONCORRENTE (Sudokus %dx%d) ---\n", SIZE, SIZE);

    // Inicializa o mutex
    if (pthread_mutex_init(&solution_mutex, NULL)) {
        printf("\nFalha ao inicializar o mutex\n");
        for (int i = 0; i < SIZE; i++) free(solution_grid[i]);
        free(solution_grid);
        return 1;
    }
    
    //Variáveis para estatísticas dos resultados
    double avg_time_total = 0.0, avg_time_easy = 0.0, avg_time_medium = 0.0, avg_time_hard = 0.0, avg_time_extreme = 0.0;
    int corrects = 0, count_easy = 0, count_medium = 0, count_hard = 0, count_extreme = 0;

    for (int test_index = 0; test_index < num_tests; test_index++) {
        printf("\n--- Teste de Indice %d (Concorrente) ---\n", test_index + 1);
        printf("Dificuldade: %s\n", get_difficulty_label(all_tests[test_index].difficulty));

        //Copiando o puzzle para a cópia imutável e para o grid de solução
        for (int i = 0; i < SIZE; i++) {
            memcpy(immutable_puzzle_grid[i], all_tests[test_index].puzzle[i], SIZE * sizeof(int));
            memcpy(solution_grid[i], all_tests[test_index].puzzle[i], SIZE * sizeof(int));
        }

        #ifdef LOG_RESULTS
        printf("Puzzle:\n");
        print_grid(solution_grid, SIZE);
        printf("\n");
        #endif

        //Reseta a flag global de solução para cada novo puzzle
        solution_found = 0;
        int row, col, thread_count = 0;

        GET_TIME(start);

        if (!find_empty_cell(solution_grid, SIZE, &row, &col)) {
            printf("Grid inicial já está resolvido.\n");
            solution_found = 1;
            corrects++;
        }
        else{
            pthread_t threads[SIZE];   //Array para guardar os IDs das threads

            for (int num = 1; num <= SIZE; num++) {
                if (is_valid(immutable_puzzle_grid, SIZE, SUBGRID_SIZE, row, col, num)) {  //Usa o grid local IMUTÁVEL para evitar condição de corrida
                    ThreadData* thread_data = (ThreadData*) malloc(sizeof(ThreadData));
                    if (thread_data == NULL) {
                        printf("Erro na alocacao de memoria para os dados da thread\n");
                        deallocate_test_cases_and_solution(SIZE, solution_grid, all_tests, num_tests);
                        return 1;
                    }
                    thread_data->size = SIZE;
                    thread_data->size_sub_grid = SUBGRID_SIZE;
                    
                    thread_data->grid = (int**) malloc(SIZE * sizeof(int*));
                    if (thread_data->grid == NULL) { free(thread_data); printf("Erro malloc\n"); return 1; }
                    for (int r = 0; r < SIZE; r++) {
                        thread_data->grid[r] = (int*) malloc(SIZE * sizeof(int));
                        if (thread_data->grid[r] == NULL) {
                            for (int k = 0; k < r; k++) free(thread_data->grid[k]);
                            free(thread_data->grid);
                            free(thread_data);
                            deallocate_test_cases_and_solution(SIZE, solution_grid, all_tests, num_tests);
                            printf("Erro malloc\n");
                            return 1;
                        }
                        memcpy(thread_data->grid[r], solution_grid[r], SIZE * sizeof(int));
                    }

                    thread_data->grid[row][col] = num;

                    //Cria a thread
                    //printf("Iniciando thread para a hipótese: Célula (%d, %d) = %d\n", row, col, num);
                    if (pthread_create(&threads[thread_count], NULL, solver_thread_func, thread_data)) {
                        fprintf(stderr, "Erro: pthread_create falhou para hipotese %d\n", num);
                        for (int k = 0; k < SIZE; k++) free(thread_data->grid[k]);
                        free(thread_data->grid);
                        free(thread_data);
                        continue;
                    }

                    thread_count++;
                }
            }
            
            printf("%d threads de trabalho criadas. Aguardando conclusao...\n", thread_count);

            //Espera todas as threads terminarem
            for (int i = 0; i < thread_count; i++) {
                if (pthread_join(threads[i], NULL)) {
                    printf("Erro ao esperar thread\n");
                    for (int k = 0; k < SIZE; k++) free(solution_grid[k]);
                    free(solution_grid);
                    deallocate_test_cases_and_solution(SIZE, solution_grid, all_tests, num_tests);
                    return 1;
                }
            }

            printf("Todas as threads terminaram.\n");
        }

        GET_TIME(end);
        delta = end - start;

        if (strcmp(get_difficulty_label(all_tests[test_index].difficulty), "Facil") == 0) {
            count_easy++;
            avg_time_easy += delta;
        } else if (strcmp(get_difficulty_label(all_tests[test_index].difficulty), "Medio") == 0) {
            count_medium++;
            avg_time_medium += delta;
        } else if (strcmp(get_difficulty_label(all_tests[test_index].difficulty), "Dificil") == 0) {
            count_hard++;
            avg_time_hard += delta;
        } else if (strcmp(get_difficulty_label(all_tests[test_index].difficulty), "Extremo") == 0) {
            count_extreme++;
            avg_time_extreme += delta;
        }
        avg_time_total += delta;

        //Verifica a corretude
        if (is_correct(SIZE, solution_grid, all_tests[test_index].solution)){
            corrects++;
            printf("Solucao correta!\n");
        }
        else {
            printf("Solucao INCORRETA!\n");
        }

        //Imprime o resultado
        #ifdef LOG_RESULTS
        if (solution_found) {
            printf("Solucao Encontrada:\n");
            print_grid(solution_grid, SIZE);
        } 
        else
            printf("Nenhuma solução foi encontrada.\n");

        if (thread_count > 0) {
            printf("Número de threads usadas: %d\n", thread_count);
        }
        printf("Tempo de execução (Tc): %.9f segundos\n", delta);
        #endif

    }

    printf("\n--- Estatísticas de Desempenho ---\n");
    printf("Solucoes corretas: %d de %d\n", corrects, num_tests);
    printf("Tempo total: %.9f segundos\n", avg_time_total);
    printf("Tempo médio total: %.9f segundos\n", avg_time_total / num_tests);
    if (count_easy > 0)
        printf("Tempo médio (Fácil): %.9f segundos\n", avg_time_easy / count_easy);
    if (count_medium > 0)
        printf("Tempo médio (Médio): %.9f segundos\n", avg_time_medium / count_medium);
    if (count_hard > 0)
        printf("Tempo médio (Difícil): %.9f segundos\n", avg_time_hard / count_hard);
    if (count_extreme > 0)
        printf("Tempo médio (Extremo): %.9f segundos\n", avg_time_extreme / count_extreme);
    

    /*
    printf("\n--- Cálculo de Aceleração (Speedup) e Eficiência ---\n");

    double seq_avg_times[4];   //seq_avg_times[0]=easy, [1]=medium, [2]=hard, [3]=extreme

    FILE *f_in_bin = fopen("seq_results.bin", "rb"); // "rb" = Read Binary
    if (f_in_bin == NULL) {
        printf("Erro: Arquivo 'seq_results.bin' nao encontrado\nExecute a versao sequencial primeiro para gerar o arquivo com resultados\n");
    } 
    else {
        //Ler os 4 doubles do arquivo binário
        size_t items_read = fread(seq_avg_times, sizeof(double), 4, f_in_bin);
        fclose(f_in_bin);

        if (items_read != 4) {
            printf("Erro: Arquivo 'seq_results.bin' está corrompido ou incompleto.\n");
        } 
        else {
            printf("| Categoria | Speedup (Ts / Tp) | Threads (N) | Eficiência (S / N) |\n");
            printf("|-----------|-------------------|-------------|--------------------|\n");

            //Calcular usando os valores do array 'seq_avg_times'
            if (count_easy > 0) {
                double conc_easy = avg_time_easy / count_easy;
                double n_easy = (double)total_threads_easy / count_easy;
                double acceleration = seq_avg_times[0] / conc_easy;
                double efficiency = acceleration / n_easy;
                printf("| Fácil     | %-17.2f | %-11.2f | %-18.2f%% |\n", speedup, n_easy, efficiency * 100);
            }
            if (count_medium > 0) {
                double tp_medium = avg_time_medium / count_medium;
                double n_medium = (double)total_threads_medium / count_medium;
                double speedup = ts_times[1] / tp_medium; // Lê do array
                double efficiency = speedup / n_medium;
                printf("| Médio     | %-17.2f | %-11.2f | %-18.2f%% |\n", speedup, n_medium, efficiency * 100);
            }
            if (count_hard > 0) {
                double tp_hard = avg_time_hard / count_hard;
                double n_hard = (double)total_threads_hard / count_hard;
                double speedup = ts_times[2] / tp_hard; // Lê do array
                double efficiency = speedup / n_hard;
                printf("| Difícil   | %-17.2f | %-11.2f | %-18.2f%% |\n", speedup, n_hard, efficiency * 100);
            }
            if (count_extreme > 0) {
                double tp_extreme = avg_time_extreme / count_extreme;
                double n_extreme = (double)total_threads_extreme / count_extreme;
                double speedup = ts_times[3] / tp_extreme; // Lê do array
                double efficiency = speedup / n_extreme;
                printf("| Extremo   | %-17.2f | %-11.2f | %-18.2f%% |\n", speedup, n_extreme, efficiency * 100);
            }
        }
    }
    */

    // Destrói o mutex e desaloca memória
    pthread_mutex_destroy(&solution_mutex);
    for (int i = 0; i < SIZE; i++) free(immutable_puzzle_grid[i]);
    free(immutable_puzzle_grid);
    deallocate_test_cases_and_solution(SIZE, solution_grid, all_tests, num_tests);

    return 0;
}