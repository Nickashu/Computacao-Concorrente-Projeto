#ifndef AUX_H
#define AUX_H
#include <stdbool.h>
#include <string.h>

// --- Defines Globais ---
#define SIZE2 9
//#define GRID_CHARS (SIZE*SIZE)
#define FILENAME "../test_cases/sudoku_test_set_16x16.txt" //Nome do arquivo com os casos de teste

// --- Estruturas Globais ---
typedef struct {
    int **puzzle;
    int **solution;
    float difficulty;
    int num_tips;
} TestCase;

typedef struct {
    bool grid[SIZE2][SIZE2][SIZE2];//para cada espaço da grid temos 9 possibilitades de cada digito estar presente ou não
    int min_cell[2];//linha e coluna da menor possibilidade
    int min_possibilities;
} Possibilities;


// --- Protótipos das Funções ---

// Funções do solver
void print_grid(int **grid, int size);

int find_empty_cell(int **grid, int size, int size_sub_grid, int *row, int *col);

int count_empty_cells(int **grid, int size);

Possibilities* find_all_possibilities(int **grid, int size, int size_sub_grid);

void change_possibilities(int **grid, int size, int size_sub_grid, int row, int col, int num, Possibilities *poss);

int is_valid(int **grid, int size, int size_sub_grid, int row, int col, int num);

// Função de verificação
int is_correct(int size, int **solution_found, int **correct_solution);

// Funções de carregamento dos casos de teste
void string_to_grid(const char* str, int size, int **grid);

int load_test_cases(TestCase** test_cases_ptr, int size);

void deallocate_test_cases_and_solution(int size, int **solution_grid, TestCase* test_cases, int num_tests);

const char *get_difficulty_label(float difficulty);

#endif