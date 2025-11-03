#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "timer.h"
#include "auxiliar.h"
#include <time.h>

// Define o tamanho do grid
#define SIZE 16
#define SUBGRID_SIZE 4
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

    if (!find_empty_cell(grid, size, size_sub_grid, &row, &col)) {
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
    double total_time = 0.0, total_time_easy = 0.0, total_time_medium = 0.0, total_time_hard = 0.0;
    int corrects = 0, count_easy = 0, count_medium = 0, count_hard = 0;

    //Agrupamento por número de threads criadas:
    double total_time_by_threads[SIZE];
    int count_by_threads[SIZE];
    for (int i = 0; i < SIZE; i++) {
        total_time_by_threads[i] = 0.0;
        count_by_threads[i] = 0;
    }

    for (int test_index = 0; test_index < num_tests; test_index++) {
        #ifdef LOG_RESULTS
        printf("\n--- Teste de Indice %d (Concorrente) ---\n", test_index + 1);
        printf("Dificuldade: %s\n", get_difficulty_label(all_tests[test_index].difficulty));
        #endif

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

        if (!find_empty_cell(solution_grid, SIZE, SUBGRID_SIZE, &row, &col)) {
            printf("Grid inicial já está resolvido.\n");
            solution_found = 1;
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
            
            #ifdef LOG_RESULTS
            printf("%d threads de trabalho criadas. Aguardando conclusao...\n", thread_count);
            #endif

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

            //printf("Todas as threads terminaram.\n");
        }

        GET_TIME(end);
        delta = end - start;

        if (strcmp(get_difficulty_label(all_tests[test_index].difficulty), "Facil") == 0) {
            count_easy++;
            total_time_easy += delta;
        } else if (strcmp(get_difficulty_label(all_tests[test_index].difficulty), "Medio") == 0) {
            count_medium++;
            total_time_medium += delta;
        } else if (strcmp(get_difficulty_label(all_tests[test_index].difficulty), "Dificil") == 0) {
            count_hard++;
            total_time_hard += delta;
        }
        total_time += delta;

        // Acumula estatísticas por número de threads
        if (thread_count > 0) {
            if (thread_count <= SIZE) {
                total_time_by_threads[thread_count - 1] += delta;
                count_by_threads[thread_count - 1]++;
            }
        }

        //Verifica a corretude
        int correct = is_correct(SIZE, solution_grid, all_tests[test_index].solution);
        if(is_correct) corrects++;

        #ifdef LOG_RESULTS
        if (correct) printf("Solucao correta!\n");
        else printf("Solucao INCORRETA!\n");

        //Imprime o resultado
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

    //Tempos concorrentes:
    double tc_total = total_time / num_tests;
    double tc_easy = count_easy > 0 ? total_time_easy / count_easy : 0.0;
    double tc_medium = count_medium > 0 ? total_time_medium / count_medium : 0.0;
    double tc_hard = count_hard > 0 ? total_time_hard / count_hard : 0.0;
    double tc_threads[SIZE];

    printf("\n--- Estatísticas de Desempenho ---\n");
    printf("Solucoes corretas: %d de %d\n", corrects, num_tests);
    printf("Tempo total: %.9f segundos\n", total_time);

    printf("Tempo médio total: %.9f segundos\n", tc_total);
    printf("Tempo médio (Fácil): %.9f segundos\n", tc_easy);
    printf("Tempo médio (Médio): %.9f segundos\n", tc_medium);
    printf("Tempo médio (Difícil): %.9f segundos\n", tc_hard);
    printf("\nAgrupando por numero de threads criadas\n");
    for(int i=0; i<SIZE; i++){
        if(count_by_threads[i] > 0){
            tc_threads[i] = total_time_by_threads[i] / count_by_threads[i];
            printf("Tempo médio (%d thread(s) criadas): %.9f segundos\n", i + 1, tc_threads[i]);
        }
        else
            tc_threads[i] = 0.0;
    }

    printf("\n--- Cálculo de Aceleração e Eficiência ---\n");
    //Lendo os tempos sequenciais do arquivo binário
    double seq_avg_times[SIZE + 4];   //[0..SIZE-1]=chutes, [SIZE]=fácil, [SIZE+1]=médio, [SIZE+2]=difícil, [SIZE+3]=total
    
    FILE *f_in_bin = fopen("seq_results.bin", "rb");
    if (f_in_bin == NULL) {
        printf("Erro: Arquivo 'seq_results.bin' nao encontrado\nExecute a versao sequencial primeiro para gerar o arquivo com resultados\n");
    } 
    else {
        size_t items_read = fread(seq_avg_times, sizeof(double), SIZE + 4, f_in_bin);
        fclose(f_in_bin);
        
        if (items_read != SIZE + 4) {
            printf("Erro: Arquivo 'seq_results.bin' está corrompido ou incompleto.\n");
        }
        else {
            double ts = seq_avg_times[SIZE+3];   //Tempo sequencial médio total
            double ts_easy = seq_avg_times[SIZE];   //Tempo sequencial agrupado por facilidade
            double ts_medium = seq_avg_times[SIZE+1];   //Tempo sequencial agrupado por facilidade
            double ts_hard = seq_avg_times[SIZE+2];   //Tempo sequencial agrupado por facilidade
            double ts_num_guesses[SIZE];   //Tempo sequencial agrupado por número de chutes iniciais
            for(int i=0; i<SIZE; i++){
                ts_num_guesses[i] = seq_avg_times[i];
            }
            
            //Não calculo a eficiência pois não temos um número fixo de threads
            printf("\nComparando tempos medios totais:\n");
            printf("Tempo sequencial (Ts): %.9f segundos\n", ts);
            printf("Tempo concorrente (Tc): %.9f segundos\n", tc_total);
            printf("Aceleracao total: %.2f\n", ts/tc_total);

            printf("\nComparando por dificuldade:\n");
            printf("--- Facil ---\n");
            printf("Sequencial: %.9f segundos\n", ts_easy);
            printf("Concorrente: %.9f segundos\n", tc_easy);
            printf("Aceleracao: %.2f\n", ts_easy/tc_easy);

            printf("\n--- Medio ---\n");
            printf("Sequencial: %.9f segundos\n", ts_medium);
            printf("Concorrente: %.9f segundos\n", tc_medium);
            printf("Aceleracao: %.2f\n", ts_medium/tc_medium);

            printf("\n--- Dificil ---\n");
            printf("Sequencial: %.9f segundos\n", ts_hard);
            printf("Concorrente: %.9f segundos\n", tc_hard);
            printf("Aceleracao: %.2f\n", ts_hard/tc_hard);

            printf("\nComparando por numero de threads/chutes iniciais:\n");
            for (int i = 0; i < SIZE; i++) {
                if (count_by_threads[i] > 0) {
                    int n_threads = i + 1;
                    printf("\n--- %d thread(s)/chute(s) ---\n", n_threads);
                    printf("Sequencial: %.9f segundos\n", ts_num_guesses[i]);
                    printf("Concorrente: %.9f segundos\n", tc_threads[i]);
                    printf("Aceleracao: %.2f\n", ts_num_guesses[i]/tc_threads[i]);
                    printf("Eficiencia: %.2f%%\n", (ts_num_guesses[i]/tc_threads[i]/n_threads)*100);
                }
            }

            //Gerando um arquivo CSV com os resultados
            time_t now;
            time(&now);
            char timestamp[20];
            strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", localtime(&now));
            
            char csv_filename[100];
            snprintf(csv_filename, sizeof(csv_filename), "resultados_conc_%s.csv", timestamp);
            
            FILE *csv_file = fopen(csv_filename, "w");
            if (csv_file == NULL) {
                printf("Erro ao criar arquivo CSV\n");
                return 1;
            }
            
            //Cabeçalho do CSV
            fprintf(csv_file, "tipo,threads,tempo_seq,tempo_conc,aceleracao,eficiencia\n");

            //Salva tempo médio total no CSV
            fprintf(csv_file, "total,0,%.9f,%.9f,%.9f,0\n", ts, tc_total, ts/tc_total);

            //Salva tempos médios por dificuldade no CSV
            fprintf(csv_file, "facil,0,%.9f,%.9f,%.9f,0\n", ts_easy, tc_easy, ts_easy/tc_easy);
            fprintf(csv_file, "medio,0,%.9f,%.9f,%.9f,0\n", ts_medium, tc_medium, ts_medium/tc_medium);
            fprintf(csv_file, "dificil,0,%.9f,%.9f,%.9f,0\n", ts_hard, tc_hard, ts_hard/tc_hard);

            //Salva tempo médio por threads/chutes no CSV
            for(int i=0; i<SIZE; i++){
                if(count_by_threads[i] > 0){
                    fprintf(csv_file, "threads,%d,%.9f,%.9f,%.9f,%.9f\n", 
                        i + 1, 
                        ts_num_guesses[i], 
                        tc_threads[i],
                        ts_num_guesses[i]/tc_threads[i],
                        (ts_num_guesses[i]/tc_threads[i]/(i+1))*100);
                }
            }

            fclose(csv_file);
            printf("\nResultados salvos em: %s\n", csv_filename);

        }
    }

    //Destrói o mutex e desaloca memória
    pthread_mutex_destroy(&solution_mutex);
    for (int i = 0; i < SIZE; i++) free(immutable_puzzle_grid[i]);
    free(immutable_puzzle_grid);
    deallocate_test_cases_and_solution(SIZE, solution_grid, all_tests, num_tests);

    return 0;
}