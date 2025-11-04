#include <stdio.h>
#include <stdlib.h>
#include "auxiliar.h"

//Funções Auxiliares
//Imprimir o grid
void print_grid(int **grid, int size) {
    for (int r = 0; r < size; r++) {
        for (int c = 0; c < size; c++)
            printf("%3d", grid[r][c]);
        printf("\n");
    }
}


//Procura por uma célula vazia (valor 0). Retorna 1 se encontrou, 0 se o grid está cheio
int find_empty_cell(int **grid, int size, int size_sub_grid, int *row, int *col) {
    for (*row = 0; *row < size; (*row)++) {
        for (*col = 0; *col < size; (*col)++) {
            if (grid[*row][*col] == 0)    //Ao sair dessa função, row e col apontarão para a primeira célula vazia encontrada
                return 1;
        }
    }
    return 0;
}


/*
//Procura por uma célula vazia (valor 0) usando heurística MRV. Retorna 1 se encontrou, 0 se o grid está cheio
int find_empty_cell(int **grid, int size, int size_sub_grid, int *row, int *col) {
    int found_empty = 0;
    int best_any = size + 1;
    int best_any_r = -1, best_any_c = -1;
    int best_ge2 = size + 1;
    int best_ge2_r = -1, best_ge2_c = -1;

    //Percorre o grid procurando a célula vazia com menos possibilidades (MRV), mas preferindo uma célula que permita >=2 chutes quando possível.
    for (int r = 0; r < size; r++) {
        for (int c = 0; c < size; c++) {
            if (grid[r][c] == 0) {
                found_empty = 1;
                //Conta quantos números são válidos para esta célula
                int possibilities = 0;
                for (int num = 1; num <= size; num++) {
                    if (is_valid(grid, size, size_sub_grid, r, c, num)) {
                        possibilities++;
                    }
                }

                //Guarda melhor MRV geral
                if (possibilities < best_any) {
                    best_any = possibilities;
                    best_any_r = r;
                    best_any_c = c;
                }

                //Guarda melhor célula com um mínimo de possibilidades
                if (possibilities >= 8 && possibilities < best_ge2) {
                    best_ge2 = possibilities;
                    best_ge2_r = r;
                    best_ge2_c = c;
                }
            }
        }
    }

    if (!found_empty) return 0;

    //if (best_ge2_r >= 0) {
    //    *row = best_ge2_r;
    //    *col = best_ge2_c;
    //} else {
        *row = best_any_r;
        *col = best_any_c;
    //}

    return 1;
}
*/

int count_empty_cells(int **grid, int size){
    int result = 0;
    for (int i = 0 ; i<size ; i++){
        for (int j = 0 ; j<size ; j++){
            if (grid [i][j] == 0){
                result++;
            }
        }
    }
    return result;
}

Possibilities* find_all_possibilities(int **grid, int size, int size_sub_grid){
    Possibilities *result = malloc(sizeof(Possibilities));
    int cell_possibilities = 0;
    int min = size + 1;
    int k = 1;
    for (int i = 0 ; i<size ; i++){//cada linha
        for (int j = 0 ; j<size ; j++){//cada coluna
            cell_possibilities = 0;
            if (grid[i][j] == 0){//checa se a célula está vazia.
                for (k = 0; k < size ; k++){//cada digito possível
                    if (is_valid(grid, size, size_sub_grid, i, j, k + 1)){
                        result->grid[i][j][k] = true;
                        cell_possibilities++;
                    }
                    else {
                        result->grid[i][j][k] = false;
                    }
                }
                if (cell_possibilities < min){//controla as informações auxiliares da struct.
                    min = cell_possibilities;
                    result->min_possibilities = min;
                    result->min_cell[0] = i;
                    result->min_cell[1] = j;
                }
            }
            
            else { //preenche a célula inteira com 0 se ela já está preenchida na grid original.
                for (k = 0 ; k < size ; k++){
                    result->grid[i][j][k] = false;
                }
            }
        }
    }
    return result;
}

void change_possibilities(int **grid, int size, int size_sub_grid, int row, int col, int num, Possibilities *poss){
    int sub_grid_row = (row)/size_sub_grid;
    int sub_grid_col = (col)/size_sub_grid;
    int min = size + 1;
    int count = 0;
    for (int i = 0 ; i < size/size_sub_grid ; i++){ //muda na subgrid
        for (int j = 0 ; j < size/size_sub_grid ; j++){
            poss->grid[i + sub_grid_row*size_sub_grid][j + sub_grid_col*size_sub_grid][num - 1] = false;
        }
    }
    for (int i = 0 ; i < size ; i++){ //muda tanto na linha quanto na coluna quanto na célula
        poss->grid[i][col][num - 1] = false;
        poss->grid[row][i][num - 1] = false;
        poss->grid[row][col][i] = false;
    }
    for (int i = 0 ; i < size ; i++){
        for (int j = 0 ; j < size ; j ++){
            count = 0;
            if (grid[i][j] == 0){//muda as informações sobre a célula mínima.
                for (int k = 0 ; k < size ; k ++) {
                    if (poss->grid[i][j][k] == true){
                        count++;
                    }
                }
                if (count < min){
                    min = count;
                    poss->min_possibilities = min;
                    poss->min_cell[0] = i;
                    poss->min_cell[1] = j;
                }        
            }
        }
    }
}


//Verifica se um número é válido em uma dada posição
int is_valid(int **grid, int size, int size_sub_grid, int row, int col, int num) {
    //Verifica a linha
    for (int c = 0; c < size; c++)
        if (grid[row][c] == num) return 0;

    //Verifica a coluna
    for (int r = 0; r < size; r++)
        if (grid[r][col] == num) return 0;

    //Verifica sub-grid
    int startRow = row - row % size_sub_grid;
    int startCol = col - col % size_sub_grid;
    for (int r = 0; r < size_sub_grid; r++)
        for (int c = 0; c < size_sub_grid; c++)
            if (grid[r + startRow][c + startCol] == num)
                return 0;
    
    return 1;
}

//Função para verificar se a solução encontrada é correta
int is_correct(int size, int **solution_found, int **correct_solution){
    for (int i=0; i<size; i++){
        for (int j=0; j<size; j++){
            if (solution_found[i][j] != correct_solution[i][j]) return 0;
        }
    }

    return 1;
}

//Função auxiliar para converter uma string (ex: "5300...") em uma matriz (para os casos de teste)
void string_to_grid(const char* str, int size, int **grid) {
    for (int r = 0; r < size; r++) {
        for (int c = 0; c < size; c++) {
            char ch = str[r * size + c];
            if (ch >= '1' && ch <= '9') {
                grid[r][c] = ch - '0';   //Converte char para int 
            } else if (ch >= 'A') {   //Para Sudokus maiores que 9x9
                grid[r][c] = ch - 'A' + 10; //'A' -> 10, 'B' -> 11,...
            } else {
                grid[r][c] = 0; //Para o '0'
            }
        }
    }
}

//Função principal de carregamento dos casos de teste. Retorna o número de testes carregados e preenche o ponteiro
int load_test_cases(TestCase** test_cases_ptr, int size) {
    FILE *fp = fopen(FILENAME, "r");
    if (fp == NULL) {
        perror("Erro ao abrir arquivo de teste");
        return 0;
    }

    //Contar quantas linhas (testes) existem
    int test_count = 0;
    char buffer[4096];  //Buffer grande para ler uma linha do arquivo de teste
    while (fgets(buffer, sizeof(buffer), fp)) {
        test_count++;
    }

    if (test_count == 0) {
        fclose(fp);
        return 0;
    }

    //Alocar memória para todos os casos de teste
    *test_cases_ptr = (TestCase*) malloc(test_count * sizeof(TestCase));
    if (*test_cases_ptr == NULL) {
        fprintf(stderr, "Erro ao alocar memória para os casos de teste\n");
        fclose(fp);
        return 0;
    }

    //Voltar ao início do arquivo e ler os dados
    rewind(fp);
    TestCase* tests = *test_cases_ptr;

    char puzzle_str[size * size + 2];
    char solution_str[size * size + 2];
    float difficulty;
    int num_tips;

    char sscanf_format[100]; // Buffer para a string de formato dinâmica
    int grid_chars = size * size;

    sprintf(sscanf_format, "%%%d[^;];%%%d[^;];%%f;%%d", grid_chars, grid_chars);  //sprintf funciona como printf, mas salva a string em uma variável
    //sscanf_format conterá algo como "%81[^;];%81[^;];%f;%d" se size for 9 ou "%256[^;];%256[^;];%f;%d" se size for 16.

    int current_test = 0;
    while (fgets(buffer, sizeof(buffer), fp)) {
        //Usa sscanf para extrair os dados da linha. Formato: [^;] significa "leia tudo ATÉ um ';'"
        int items = sscanf(buffer, sscanf_format, puzzle_str, solution_str, &difficulty, &num_tips);
        //printf("items: %d\n", items);
        
        if (items == 4) {
            //Aloca memória para o puzzle e a solução
            tests[current_test].puzzle = (int**) malloc(size * sizeof(int*));
            tests[current_test].solution = (int**) malloc(size * sizeof(int*));
            for (int i = 0; i < size; i++) {
                tests[current_test].puzzle[i] = (int*) malloc(size * sizeof(int));
                tests[current_test].solution[i] = (int*) malloc(size * sizeof(int));
            }
            string_to_grid(puzzle_str, size, tests[current_test].puzzle);
            string_to_grid(solution_str, size, tests[current_test].solution);
            tests[current_test].difficulty = difficulty;
            tests[current_test].num_tips = num_tips;
            current_test++;
            //printf("Dificuldade: %f", difficulty);
        }
    }

    fclose(fp);

    //Se nenhum teste válido foi carregado, libera a memória alocada e retorna 0
    if (current_test == 0) {
        free(*test_cases_ptr);
        *test_cases_ptr = NULL;
        return 0;
    }

    //Se o número de testes válidos for menor que o número de linhas contadas, reduz o bloco alocado para economizar memória
    if (current_test < test_count) {
        TestCase* shrunk = (TestCase*) realloc(*test_cases_ptr, current_test * sizeof(TestCase));
        if (shrunk != NULL) {
            *test_cases_ptr = shrunk;
        }
    }

    return current_test;
}

void deallocate_test_cases_and_solution(int size, int **solution_grid, TestCase* test_cases, int num_tests) {
    for (int i = 0; i < num_tests; i++) {
        for (int j = 0; j < size; j++) {
            free(test_cases[i].puzzle[j]);
            free(test_cases[i].solution[j]);
        }
        free(test_cases[i].puzzle);
        free(test_cases[i].solution);
    }
    free(test_cases);
    for (int i = 0; i < size; i++) {
        free(solution_grid[i]);
    }
    free(solution_grid);
}

const char *get_difficulty_label(float difficulty) {
    if (difficulty < 2.0) {
        return "Facil";
    } 
    else if (difficulty < 5.0) {
        return "Medio";
    } 
    else {
        return "Dificil";
    }
}