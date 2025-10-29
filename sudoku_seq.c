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

    if (!find_empty_cell(grid, size, &row, &col)) {   //Se não há células vazias, o Sudoku está resolvido (sucesso)
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
    double start, end, delta;   //Variáveis para controle de tempo

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
    int num_tests = load_test_cases(&all_tests, SIZE);

    if (num_tests == 0) {
        printf("Nenhum caso de teste foi carregado.\n");
        return 1;
    }

    printf("Sucesso! %d casos de teste carregados da memória.\n", num_tests);
    printf("--- Solucionador de Sudoku SEQUENCIAL (Sudokus %dx%d) ---\n", SIZE, SIZE);

    //Variáveis para estatísticas dos resultados
    double avg_time_total = 0.0, avg_time_easy = 0.0, avg_time_medium = 0.0, avg_time_hard = 0.0, avg_time_extreme = 0.0;
    int corrects = 0, count_easy = 0, count_medium = 0, count_hard = 0, count_extreme = 0;
    
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

        GET_TIME(start);  //Começando a contagem de tempo para o cálculo sequencial
        int solved = solve_sequential(solution_grid, SIZE, SUBGRID_SIZE);
        GET_TIME(end);  //Terminando a contagem de tempo para o sequencial
        double delta = end - start;

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
            printf("Solucao correta!\n");
            corrects++;
        }
        else {
            printf("Solucao INCORRETA!\n");
        }

        //Imprime o resultado
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
    

    //Salvando os resultados sequenciais em um arquivo
    double seq_avg_times[4];
    seq_avg_times[0] = (count_easy > 0) ? (avg_time_easy / count_easy) : 0.0;
    seq_avg_times[1] = (count_medium > 0) ? (avg_time_medium / count_medium) : 0.0;
    seq_avg_times[2] = (count_hard > 0) ? (avg_time_hard / count_hard) : 0.0;
    seq_avg_times[3] = (count_extreme > 0) ? (avg_time_extreme / count_extreme) : 0.0;

    FILE *f_out_bin = fopen("seq_results.bin", "wb");  //"wb" = Write Binary
    if (f_out_bin == NULL) {
        printf("Erro ao salvar os resultados sequenciais binários.\n");
    } 
    else {
        fwrite(seq_avg_times, sizeof(double), 4, f_out_bin);
        fclose(f_out_bin);
        printf("\nTempos médios sequenciais salvos em 'seq_results.bin'\n");
    }

    //Desaloca memória
    deallocate_test_cases_and_solution(SIZE, solution_grid, all_tests, num_tests);

    return 0;
}