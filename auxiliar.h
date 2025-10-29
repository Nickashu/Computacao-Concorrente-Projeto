#ifndef AUX_H
#define AUX_H

// --- Defines Globais ---
//#define SIZE 9
//#define GRID_CHARS (SIZE*SIZE)
#define FILENAME "sudoku_test_set_9x9.txt" //Nome do arquivo com os casos de teste

// --- Estruturas Globais ---
typedef struct {
    int **puzzle;
    int **solution;
    float difficulty;
} TestCase;


// --- Protótipos das Funções ---

// Funções do solver
void print_grid(int **grid, int size);

int find_empty_cell(int **grid, int size, int *row, int *col);

int is_valid(int **grid, int size, int size_sub_grid, int row, int col, int num);

// Função de verificação
int is_correct(int size, int **solution_found, int **correct_solution);

// Funções de carregamento dos casos de teste
void string_to_grid(const char* str, int size, int **grid);

int load_test_cases(TestCase** test_cases_ptr, int size);

void deallocate_test_cases_and_solution(int size, int **solution_grid, TestCase* test_cases, int num_tests);

const char *get_difficulty_label(float difficulty);

#endif