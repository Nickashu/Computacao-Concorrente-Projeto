#include <stdio.h>
#include <stdlib.h>
#include "aux.h"

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
int find_empty_cell(int **grid, int size, int *row, int *col) {
    for (*row = 0; *row < size; (*row)++) {
        for (*col = 0; *col < size; (*col)++) {
            if (grid[*row][*col] == 0)    //Ao sair dessa função, row e col apontarão para a primeira célula vazia encontrada
                return 1;
        }
    }
    return 0;
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
    char buffer[256]; //Buffer grande para ler uma linha
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

    char sscanf_format[100]; // Buffer para a string de formato dinâmica
    int grid_chars = size * size;

    sprintf(sscanf_format, "%%%d[^;];%%%d[^;];%%f;", grid_chars, grid_chars);  //sprintf funciona como printf, mas salva a string em uma variável

    //sscanf_format conterá algo como "%81[^;];%81[^;];%f;" se size for 9 ou "%256[^;];%256[^;];%f;" se size for 16.

    int current_test = 0;
    while (fgets(buffer, sizeof(buffer), fp)) {
        //Usa sscanf para extrair os dados da linha. Formato: [^;] significa "leia tudo ATÉ um ';'"
        int items = sscanf(buffer, sscanf_format, puzzle_str, solution_str, &difficulty);
        //printf("items: %d\n", items);
        
        if (items == 3) {
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
            current_test++;
            //printf("Dificuldade: %f", difficulty);
        }
    }

    fclose(fp);
    return test_count;
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
    else if (difficulty < 4.0) {
        return "Medio";
    } 
    else if (difficulty < 7.0) {
        return "Dificil";
    }
    else {
        return "Extremo";
    }
}