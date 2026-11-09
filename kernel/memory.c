//GRR20234398   Henrico José Cornelio

// PingPongOS - PingPong Operating System

// Este arquivo PODE/DEVE ser alterado.

// Alocador básico de memória heap.

// somente para a implementação trivial
#include <stdlib.h>
#include "memory.h"

#ifndef ERROR
#define ERROR -1
#define NOERROR 0
#endif

// implementação trivial, a ser substituída
void *mem_alloc(int size)
{
    return malloc(size);
}

int mem_free(void *addr)
{
    if (addr == NULL)
        return ERROR;

    free(addr);
    return NOERROR;
}

void mem_init()
{
}

void mem_term()
{
}

int mem_size()
{
    return 0;
}

int mem_avail()
{
    return 0;
}

void mem_report()
{
}

