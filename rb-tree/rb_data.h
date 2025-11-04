/*
 * Copyright (c) 2019 xieqing. https://github.com/xieqing
 * May be freely redistributed, but copyright notice must be retained.
 */
#include "../auxiliar.h"
#ifndef _RB_DATA_HEADER
#define _RB_DATA_HEADER

typedef struct {
	int key;
	int size;
	int size_sub_grid;
	int **grid;
	Possibilities *poss;
} mydata;

mydata *makedata(int key, int size, int size_sub_grid, int **grid, Possibilities *poss);
int compare_func(const void *d1, const void *d2);
void destroy_func(void *d);
void print_func(void *d);
void print_char_func(void *d);

#endif /* _RB_DATA_HEADER */

