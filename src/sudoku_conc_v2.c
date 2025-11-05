#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "timer.h"
#include "auxiliar.h"
#include <time.h>
#include "rb-tree/rb.h"
#include "rb-tree/rb_data.h"
#include <semaphore.h>
#include <stdbool.h>


// Define o tamanho do grid
#define SIZE 9
#define SUBGRID_SIZE 3
#define NTHREADS 16
#define SUCESSO (int *) 1

//#define LOG_RESULTS   //Comentar para desativar o log dos resultados detalhados

//Structs e Variáveis Globais

//Variáveis globais compartilhadas
short int solution_found = 0;   //Flag: 1 se a solução foi encontrada
int **solution_grid;         //Matriz onde a solução final será armazenada
int qtd_buffer = 0;
rbtree *rbt; //árvore de busca binária onde ficará a fila

time_t segundos_desde_epoca;
struct tm *tempo_local;
sem_t buffer_control;
pthread_mutex_t mutex;
pthread_cond_t cond_retirar;

void* solver_thread_func(void* arg) {
    mydata *data;
    mydata *newdata;
    int row_min_poss;
    int col_min_poss;
    int min_poss;
    
    while(!solution_found){
        //sessão de controle do buffer
        pthread_mutex_lock(&mutex);
        while(qtd_buffer == 0){
            pthread_cond_wait(&cond_retirar, &mutex);
            if(solution_found){
                pthread_cond_broadcast(&cond_retirar);
                pthread_mutex_unlock(&mutex);
                pthread_exit(SUCESSO);
            }
        }
        if(solution_found){//outra thread achou a solução
            pthread_cond_broadcast(&cond_retirar);
            pthread_mutex_unlock(&mutex);
            pthread_exit(SUCESSO);
        }

        //printf("\nrbt min: %d", rbt->min->data->key);
        data = rb_delete(rbt, rbt->min, 1);
        if (data == NULL){
            printf("\nerro ao pegar os dados");
            pthread_mutex_unlock(&mutex);
            pthread_cond_broadcast(&cond_retirar);
            pthread_exit(NULL);
        }
        //printf("\nvalores de data: ");
        //printf("\ngrid key: %d\n", data->key);
        qtd_buffer--;
        if (qtd_buffer > 0){
            pthread_cond_broadcast(&cond_retirar);
        }
        pthread_mutex_unlock(&mutex);
        //fim da sessão de controle do buffer.

        //print_grid(data->grid, data->size);
        row_min_poss = data->poss->min_cell[0];
        col_min_poss = data->poss->min_cell[1];
        min_poss = data->poss->min_possibilities;
        //printf("\nvalores a testar:\n row_min_poss: %d\n col_min_poss: %d\nmin_poss: %d", row_min_poss, col_min_poss, min_poss);
        Possibilities* new_poss[min_poss];
        int last_put_number[data->size];
        int idx = 0;
        for (int j = 0 ; j < data->size ; j++){
            if (data->poss->grid[row_min_poss][col_min_poss][j] == true){
                if (data->key == 1){//caso de sucesso
                    printf("\ncheguei no fim");
                    data->grid[row_min_poss][col_min_poss] = j + 1;
                    pthread_mutex_lock(&mutex);
                    solution_found = true;
                    for (int r = 0; r < data->size; r++) {  //Copia a solução local para a global
                        memcpy(solution_grid[r], data->grid[r], data->size * sizeof(int));
                    }
                    pthread_cond_broadcast(&cond_retirar);
                    pthread_mutex_unlock(&mutex);
                    destroy_func(data);
                    pthread_exit(SUCESSO);
                }
                new_poss[idx] = malloc(sizeof(Possibilities));
                if(new_poss[idx] == NULL){
                    printf("\n erro no malloc");
                    pthread_cond_broadcast(&cond_retirar);
                    destroy_func(data);
                    pthread_exit(NULL);
                }
                memcpy(new_poss[idx], data->poss, sizeof(Possibilities));
                data->grid[row_min_poss][col_min_poss] = j + 1;
                last_put_number[idx] = j + 1;
                change_possibilities(data->grid, data->size, data->size_sub_grid, row_min_poss, col_min_poss, j + 1, new_poss[idx]);
                data->grid[row_min_poss][col_min_poss] = 0;
                if (new_poss[idx]->min_possibilities == 0){
                    free(new_poss[idx]);
                    new_poss[idx] = NULL;
                }
                idx++;
            }
        }
        //acesso ao buffer
        pthread_mutex_lock(&mutex);

        if(solution_found){
            pthread_cond_broadcast(&cond_retirar);
            pthread_mutex_unlock(&mutex);
            pthread_exit(SUCESSO);
        }
        //sem_wait(&buffer_control);
        for (int i = 0 ; i < idx ; i ++){
            if (new_poss[i] != NULL)
            {
                data->grid[row_min_poss][col_min_poss] = last_put_number[i];                
                if((newdata = makedata((data->key) - 1, data->size, data->size_sub_grid, data->grid, new_poss[i])) == NULL || rb_insert(rbt, newdata) == NULL){
                    fprintf(stderr, "\nsem memória pra inserir na fila");
                    pthread_mutex_unlock(&mutex);
                    pthread_cond_broadcast(&cond_retirar);
                    pthread_exit(NULL);
                }
                qtd_buffer++;
            }

        }
        if(qtd_buffer > 0){
            pthread_cond_broadcast(&cond_retirar);
        }
        pthread_mutex_unlock(&mutex);
        //fim do acesso ao buffer;
        destroy_func(data);
        //printf("\nteste");
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
}


int main(int argc, char *argv[])
{
    /* if (sem_init(&buffer_control, 0, 1)){
        printf("\nerro ao inicializar o semáforo");
        return 1;
    } */

    
    if (pthread_mutex_init(&mutex, NULL)){
        printf("\nerro ao inicializar o mutex");
        return 1;
    }
    if (pthread_cond_init(&cond_retirar, NULL)){
        printf("\nerro ao inicializar a condição");
        return 1;
    }
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
        
        /* create a red-black tree */
        if ((rbt = rb_create(compare_func, destroy_func)) == NULL) {
            fprintf(stderr, "create red-black tree failed\n");
            return 1;
        }


        mydata *data;
        int key;

        //Reseta a flag global de solução para cada novo puzzle
        solution_found = 0;
        int row, col, thread_count = 0;
        qtd_buffer = 0;
        
        GET_TIME(start);

        if (!find_empty_cell(solution_grid, SIZE, &row, &col)) {
            printf("Grid inicial já está resolvido.\n");
            solution_found = 1;
            corrects++;
            rb_destroy(rbt);
        }
        else{
            pthread_t threads[NTHREADS];   //Array para guardar os IDs das threads
            key = count_empty_cells(immutable_puzzle_grid, SIZE);
            printf("\nkey: %d", key);
            Possibilities* initial_poss = find_all_possibilities(immutable_puzzle_grid, SIZE, SUBGRID_SIZE);
            printf("\npossibilidades celula com mínimo: %d,%d", initial_poss->min_cell[0], initial_poss->min_cell[1]);
            printf("\nminimo qtd: %d", initial_poss->min_possibilities);
            printf("\npossibilidades na célula: %d", initial_poss->grid[initial_poss->min_cell[0]][initial_poss->min_cell[1]][0]);
            for (int i = 1 ; i<SIZE ; i++){
                printf("%d", initial_poss->grid[initial_poss->min_cell[0]][initial_poss->min_cell[1]][i]);
            }
            if ((data = makedata(key, SIZE, SUBGRID_SIZE, immutable_puzzle_grid, initial_poss)) == NULL || rb_insert(rbt, data) == NULL) {
                fprintf(stderr, "insert grid %d: out of memory\n", test_index);
                deallocate_test_cases_and_solution(SIZE, solution_grid, all_tests, num_tests);
                break;
            }
            qtd_buffer++;
            for (int num = 1; num <= NTHREADS; num++) {
                if (pthread_create(&threads[thread_count], NULL, solver_thread_func, NULL)) {
                    fprintf(stderr, "Erro: pthread_create %d falhou\n", num);

                    continue;
                }

                thread_count++;
            }
            
            printf("\n%d threads de trabalho criadas. Aguardando conclusao...\n", thread_count);

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

        rb_destroy(rbt);

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

    for (int i = 0; i < SIZE; i++) free(immutable_puzzle_grid[i]);
    free(immutable_puzzle_grid);
    deallocate_test_cases_and_solution(SIZE, solution_grid, all_tests, num_tests);

    return 0;
}