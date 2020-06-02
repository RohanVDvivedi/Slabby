#ifndef CAPP_H
#define CAPP_H

#include<stdio.h>

typedef struct temp temp;
struct temp
{
	int s;
	char str[30];
};

void print_temp_test(temp t);

#endif