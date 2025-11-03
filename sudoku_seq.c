#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timer.h"
#include "auxiliar.h"

//Tamanho do grid
#define SIZE 9
#define SUBGRID_SIZE 3
//#define LOG_RESULTS   //Comentar para desativar o log dos resultados detalhados

//Solucionador Sequencial (Backtracking)
int solve_sequential(int **grid, int size, int size_sub_grid) {
    int row, col;

    if (!find_empty_cell(grid, size, size_sub_grid, &row, &col)) {   //Se não há células vazias, o Sudoku está resolvido (sucesso)
        return 1;
    }
    //printf("Tentando resolver célula vazia em (%d, %d)\n", row, col);

    //Tenta números de 1 a 9
    for (int num = 1; num <= SIZE; num++) {
        if (is_valid(grid, size, size_sub_grid, row, col, num)) {
            grid[row][col] = num;
            if (solve_sequential(grid, size, size_sub_grid)) {
                return 1; //Sucesso
            }
            grid[row][col] = 0; //Falhou, desfaz (backtrack)
        }
    }

    return 0;   //Nenhuma solução encontrada neste ramo
}


//Função main
int main() {
    double start, end;   //Variáveis para controle de tempo

    //Aloca memória para o grid de solução
    int **solution_grid = (int **) malloc(SIZE * sizeof(int *));
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

    TestCase* all_tests = NULL;   //Testes de sudokus
    int num_tests = load_test_cases(&all_tests, SIZE);  //Carregando os testes da memória

    if (num_tests == 0) {
        printf("Nenhum caso de teste foi carregado.\n");
        return 1;
    }

    printf("Sucesso! %d casos de teste carregados da memória.\n", num_tests);
    printf("--- Solucionador de Sudoku SEQUENCIAL (Sudokus %dx%d) ---\n", SIZE, SIZE);

    //Variáveis para estatísticas dos resultados
    double total_time = 0.0, total_time_easy = 0.0, total_time_medium = 0.0, total_time_hard = 0.0;
    double total_time_guesses[SIZE];  // array indexado por número de chutes-1 (1 chute -> índice 0)
    int corrects = 0, count_easy = 0, count_medium = 0, count_hard = 0;
    int count_guesses[SIZE];   //Lista para identificar a quantidade de sudokus por número de chutes iniciais
    
    // Inicializa arrays de contagem/tempo
    for(int k = 0; k < SIZE; k++) {
        total_time_guesses[k] = 0.0;
        count_guesses[k] = 0;
    }
    for(int k=0; k<SIZE; k++){
        count_guesses[k] = 0;
    }
    
    for (int test_index=0; test_index < num_tests; test_index++){
        printf("\n--- Exibindo Teste de Índice %d (Sequencial) ---\n", test_index + 1);
        printf("Dificuldade: %s\n", get_difficulty_label(all_tests[test_index].difficulty));

        for (int i = 0; i < SIZE; i++) {
            memcpy(solution_grid[i], all_tests[test_index].puzzle[i], SIZE * sizeof(int));
        }
        
        #ifdef LOG_RESULTS
        printf("Puzzle:\n");
        print_grid(solution_grid, SIZE);
        #endif

        // Verificando quantos chutes iniciais teremos: busca a primeira célula vazia
        int guesses = 0, r, c;
        if (find_empty_cell(solution_grid, SIZE, SUBGRID_SIZE, &r, &c)) {
            for (int num = 1; num <= SIZE; num++) {
                if (is_valid(solution_grid, SIZE, SUBGRID_SIZE, r, c, num)) {
                    guesses++;
                }
            }
            if (guesses > 0) {
                count_guesses[guesses-1]++;
            }
        } else {
            // Se não há célula vazia (já está resolvido), mantemos guesses = 0
            guesses = 0;
        }

        GET_TIME(start);  //Começando a contagem de tempo para a resolução sequencial
        int solved = solve_sequential(solution_grid, SIZE, SUBGRID_SIZE);
        GET_TIME(end);  //Terminando a contagem de tempo para a resolução sequencial
        double delta = end - start;

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
        if (guesses > 0) {
            total_time_guesses[guesses-1] += delta;
        }

        //Verificando a corretude
        if (is_correct(SIZE, solution_grid, all_tests[test_index].solution)){
            printf("Solucao correta!\n");
            corrects++;
        }
        else {
            printf("Solucao INCORRETA!\n");
        }

        //Imprime detalhes do resultado
        #ifdef LOG_RESULTS
        if (solved) {
            printf("\nSolucao Encontrada:\n");
            print_grid(solution_grid, SIZE);
        } 
        else
            printf("\nNenhuma solucao foi encontrada.\n");
        
        printf("Tempo de execução (Ts): %.9f segundos\n", delta);
        #endif
    }

    printf("\n--- Estatísticas de Desempenho ---\n");
    printf("Solucoes corretas: %d de %d\n", corrects, num_tests);
    printf("Tempo total: %.9f segundos\n", total_time);
    printf("Tempo médio total: %.9f segundos\n", total_time / num_tests);
    if (count_easy > 0)
        printf("Tempo médio (Fácil): %.9f segundos\n", total_time_easy / count_easy);
    if (count_medium > 0)
        printf("Tempo médio (Médio): %.9f segundos\n", total_time_medium / count_medium);
    if (count_hard > 0)
        printf("Tempo médio (Difícil): %.9f segundos\n", total_time_hard / count_hard);
    
    printf("\nAgrupando por numero de chutes iniciais\n");
    for(int i=0; i<SIZE; i++){
        if(count_guesses[i] != 0)
            printf("Tempo médio (%d chute(s) inicial(is)): %.9f segundos\n", i+1, total_time_guesses[i] / count_guesses[i]);
    }
    
    //Salvando os resultados sequenciais em um arquivo binário  (agrupados por dificuldades e número de chutes iniciais)
    double seq_avg_times[4 + SIZE];
    for(int i=0; i<SIZE; i++){  //Tempos médios por chutes iniciais
        seq_avg_times[i] = (count_guesses[i] > 0) ? (total_time_guesses[i] / count_guesses[i]) : 0.0;
    }
    seq_avg_times[SIZE] = (count_easy > 0) ? (total_time_easy / count_easy) : 0.0;   //Tempo médio das fáceis
    seq_avg_times[SIZE + 1] = (count_medium > 0) ? (total_time_medium / count_medium) : 0.0;   //Tempo médio das médias
    seq_avg_times[SIZE + 2] = (count_hard > 0) ? (total_time_hard / count_hard) : 0.0;   //Tempo médio das difíceis
    seq_avg_times[SIZE + 3] = total_time / num_tests;   //Tempo médio total

    FILE *f_out_bin = fopen("seq_results.bin", "wb");
    if (f_out_bin == NULL) {
        printf("Erro ao salvar os resultados sequenciais binários.\n");
    } 
    else {
        fwrite(seq_avg_times, sizeof(double), SIZE + 4, f_out_bin);
        fclose(f_out_bin);
        printf("\nTempos médios sequenciais salvos em 'seq_results.bin'\n");
    }

    //Desaloca memória
    deallocate_test_cases_and_solution(SIZE, solution_grid, all_tests, num_tests);

    return 0;
}