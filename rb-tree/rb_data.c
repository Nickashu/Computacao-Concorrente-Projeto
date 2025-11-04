/*
 * Copyright (c) 2019 xieqing. https://github.com/xieqing
 * May be freely redistributed, but copyright notice must be retained.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "rb_data.h"

mydata *makedata(int key, int size, int size_sub_grid, int **grid, Possibilities *poss)
{
    mydata *p;

    // Allocate memory for the struct itself
    p = (mydata *) malloc(sizeof(mydata));
    if (p == NULL) {
        printf("\n erro de memória alocando dados");
        return NULL;
    }
    
    p->key = key;
    p->size = size;
    p->size_sub_grid = size_sub_grid;
    
    // Allocate memory for the array of pointers
    p->grid = (int **) malloc(size * sizeof(int *));
    if (p->grid == NULL) {
        printf("\n erro de memória alocando array de ponteiros");
        free(p);  // Clean up already allocated memory
        return NULL;
    }
    
    // Allocate memory for each row
    for (int i = 0; i < size; i++) {
        p->grid[i] = (int *) malloc(size * sizeof(int));
        if (p->grid[i] == NULL) {
            printf("\n erro de memória alocando para a grid.");
            // Clean up previously allocated rows
            for (int k = 0; k < i; k++) {
                free(p->grid[k]);
            }
            free(p->grid);
            free(p);
            return NULL;
        }
        
        // Copy datafor (int i = 0; i < SIZE; i++) {
    }
    for (int j = 0; j < size; j++) {
        memcpy(p->grid[j], grid[j], size * sizeof(int));
    }
    assert(poss != NULL);

    p->poss = poss;

    return p;
}

int compare_func(const void *d1, const void *d2)
{
	mydata *p1, *p2;
	
	assert(d1 != NULL);
	assert(d2 != NULL);
	
	p1 = (mydata *) d1;
	p2 = (mydata *) d2;
	if (p1->key == p2->key)
		return 0;
	else if (p1->key > p2->key)
		return 1;
	else
		return -1;
}

void destroy_func(void *d)
{
	mydata *p;
    assert(d != NULL);
    p = (mydata *) d;
    if (p->grid != NULL) {
        for (int i = 0; i < p->size; i++) {
            free(p->grid[i]);
        }
        free(p->grid);
    }
    
    // Free possibilities if it exists
    if (p->poss != NULL) {
        free(p->poss);
    }
    
    free(p);
}

void print_func(void *d)
{
	mydata *p;
	
	assert(d != NULL);
	
	p = (mydata *) d;
	printf("%d", p->key);
}

void print_char_func(void *d)
{
	mydata *p;
	
	assert(d != NULL);
	
	p = (mydata *) d;
	printf("%c", p->key & 127);
}