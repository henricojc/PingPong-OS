//GRR20234398 Henrico José Cornelio

// PingPongOS - PingPong Operating System
// © Prof. Carlos A. Maziero, DINF UFPR
// Versão 2.1 -- 06/2026

// Este arquivo PODE/DEVE ser alterado.

// Descritor de tarefas (TCB - Task Control Block).

#ifndef __PPOS_TCB__
#define __PPOS_TCB__

#include "ctx.h"

#define TASK_READY      0
#define TASK_RUNNING    1
#define TASK_SUSPENDED  2
#define TASK_TERMINATED 3

struct queue_t;

// Task Control Block (TCB), infos sobre uma tarefa
struct task_t
{
    int id;                  // identificador da tarefa
    char *name;              // nome da tarefa
    struct ctx_t context;    // contexto da tarefa
    int status;              // pronta, executando, ...

    void *stack;             // pilha da tarefa
    unsigned long stack_size;

    int vg_id;               // ID da pilha da tarefa no Valgrind

    int static_prio;         // prioridade estática
    int dynamic_prio;        // prioridade dinâmica

    int quantum;       // ticks restantes do quantum
    int is_user;       // 1 = tarefa de usuário, 0 = tarefa de sistema

    unsigned int creation_time;   // instante de criação
    unsigned int cpu_time;        // tempo total usando a CPU
    unsigned int last_cpu_start;  // início da ativação atual
    unsigned int activations;     // número de ativações
    unsigned int wake_time;

    int exit_code;           // código de encerramento

    struct queue_t *wait_queue;       // tarefas esperando esta tarefa terminar
    struct queue_t *suspended_queue;  // fila em que esta tarefa está suspensa

    struct task_t *prev;     // tarefa anterior
    struct task_t *next;     // próxima tarefa
    struct task_t *parent;
};

#endif
