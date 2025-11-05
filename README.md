# Resolvedor de Sudoku Concorrente
Repositório para o projeto de implementação da disciplina de Computação Concorrente. O objetivo é implementar e comparar a performance de um resolvedor de Sudoku sequencial contra uma versão concorrente (ambos utilizando a técnica de Backtracking).

O programa é capaz de resolver puzzles lendo-os de um arquivo de teste e calculando métricas de desempenho como aceleração e eficiência.

## Requisitos

* Um compilador C (ex: `gcc`)
* Biblioteca `Pthreads` (padrão em sistemas POSIX como Linux e macOS)

## Configuração dos Testes

Antes de compilar, você pode definir qual conjunto de testes (9x9 ou 16x16) o programa deve usar.

**É obrigatório que os três arquivos (`sudoku_seq.c`, `sudoku_conc.c` e `auxiliar.h`) estejam com a mesma configuração.**

1.  **Em `src/auxiliar.h`**, defina o caminho do arquivo de teste:
    * **Para 9x9:** `#define FILENAME "../test_cases/sudoku_test_set_9x9.txt"`
    * **Para 16x16:** `#define FILENAME "../test_cases/sudoku_test_set_16x16.txt"`

2.  **Em `src/sudoku_seq.c` E `src/sudoku_conc.c`**, ajuste os tamanhos da grade:
    * **Para 9x9:**
        ```c
        #define SIZE 9
        #define SUBGRID_SIZE 3
        ```
    * **Para 16x16:**
        ```c
        #define SIZE 16
        #define SUBGRID_SIZE 4
        ```

## Como Compilar e Executar

O fluxo de execução foi projetado para coletar e comparar métricas de performance, e pode ser visto nos passos abaixo:

1.  **Navegue até a pasta `src`:**
    ```bash
    cd src
    ```

2.  **Compile os dois programas:**
    * **Sequencial:**
        ```bash
        gcc -o sudoku_seq sudoku_seq.c auxiliar.c
        ```
    * **Concorrente:**
        ```bash
        gcc -o sudoku_conc sudoku_conc.c auxiliar.c
        ```

3.  **Execute a versão Sequencial:**
    ```bash
    ./sudoku_seq
    ```
    > **Importante:** É necessário rodar a versão sequencial primeiro. Esta execução irá resolver os Sudokus e salvar os tempos de execução em um arquivo binário (`seq_results.bin`).

4.  **Execute a versão Concorrente (2º Passo):**
    ```bash
    ./sudoku_conc
    ```

    Ao ser executado, o programa concorrente irá:
    1.  Ler o arquivo `seq_results.bin` para obter os tempos sequenciais.
    2.  Resolver os mesmos Sudokus utilizando a técnica de concorrência.
    3.  Imprimir na tela os resultados com tempos de execução, aceleração e eficiência.
