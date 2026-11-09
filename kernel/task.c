//GRR20234398 Henrico José Cornelio

// PingPongOS - PingPong Operating System

// Este arquivo PODE/DEVE ser alterado.

// Gerência básica de tarefas.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <valgrind/valgrind.h>
#include "task.h"
#include "tcb.h"
#include "dispatcher.h"
#include "hardware/cpu.h"
#include "lib/pplibc.h"
#include "semaphore.h"

#define STACKSIZE (32 * 1024)

static struct task_t *current_task = NULL;
struct task_t *task_kernel;
extern struct queue_t *ready_queue;
extern struct queue_t *sleeping_queue;
extern unsigned int time();
static int next_id = 1;
extern int user_tasks;


void task_init()
{
    task_kernel = malloc(sizeof(struct task_t));
    current_task = task_kernel;

    current_task->id = 0;
    current_task->name = "kernel";
    current_task->status = TASK_RUNNING;

    current_task->stack = NULL;
    current_task->stack_size = 0;
    current_task->exit_code = 0;

    task_kernel->quantum = 0;
    task_kernel->is_user = 0;

    task_kernel->creation_time = time();
    task_kernel->cpu_time = 0;
    task_kernel->last_cpu_start = time();
    task_kernel->activations = 1;

    task_kernel->wait_queue = NULL;
    task_kernel->suspended_queue = NULL;

    current_task->prev = NULL;
    current_task->next = NULL;
    current_task->parent = NULL;

    next_id = 1;
}
struct task_t *task_create(char *name,
                           void (*entry)(void *),
                           void *arg)
{
    struct task_t *task;

    if (entry == NULL)
        return NULL;

    task = malloc(sizeof(struct task_t));

    if (task == NULL)
        return NULL;

    // o nome pode ser NULL
    if (name != NULL)
    {
        task->name = malloc(strlen(name) + 1);

        if (task->name == NULL)
        {
            free(task);
            return NULL;
        }

        strcpy(task->name, name);
    }
    else
    {
        task->name = NULL;
    }

    task->id = next_id++;

    task->stack_size = STACKSIZE;
    task->stack = malloc(task->stack_size);

    if (task->stack == NULL)
    {
        if (task->name != NULL)
            free(task->name);
        free(task);
        return NULL;
    }
    task->vg_id = VALGRIND_STACK_REGISTER(
        task->stack,
        (char *)task->stack + task->stack_size
    );

    // cria o contexto inicial da tarefa
    if (ctx_create(&task->context,
                   entry,
                   arg,
                   task->stack,
                   task->stack_size) != NOERROR)
    {
        free(task->stack);

        if (task->name != NULL)
            free(task->name);

        free(task);

        return NULL;
    }

    task->status = TASK_READY;
    task->exit_code = 0;
    task->static_prio = 0;
    task->dynamic_prio = 0;
    task->quantum = 10;
    task->is_user = 1;

    task->creation_time = time();
    task->cpu_time = 0;
    task->last_cpu_start = time();
    task->activations++;

    task->prev = NULL;
    task->next = NULL;

    task->wait_queue = queue_create();
    if (task->wait_queue == NULL)
    {
        VALGRIND_STACK_DEREGISTER(task->vg_id);
        free(task->stack);

        if (task->name != NULL)
            free(task->name);

        free(task);
        return NULL;
    }
    task->suspended_queue = NULL;

    // quem está executando neste momento é o criador da nova tarefa
    task->parent = current_task;
    queue_add(ready_queue, task);
    user_tasks++;

    return task;
}

int task_id(struct task_t *task)
{
    if (task == NULL)
        task = current_task;

    if (task == NULL)
        return ERROR;

    return task->id;
}

char *task_name(struct task_t *task)
{
    if (task == NULL)
        task = current_task;

    if (task == NULL)
        return NULL;

    return task->name;
}

struct task_t *task_current(void)
{
    return current_task;
}

// task = null volta para a tarefa que criou a tarefa atual
int task_switch(struct task_t *task)
{
    struct task_t *previous;
    struct task_t *next;
    unsigned int now;
    int status;

    previous = current_task;

    if (previous == NULL)
        return ERROR;

    if (task == NULL)
        next = task_kernel;
    else
        next = task;

    if (next == previous)
        return ERROR;

    now = time();
    previous->cpu_time += now - previous->last_cpu_start;
    next->last_cpu_start = now;
    next->activations++;

    // quando a tarefa anterior voltar a executar, current_task terá sido atualizado pela tarefa
    // que fez a troca de volta.
    current_task = next;
    status = ctx_switch(&previous->context, &next->context);

    if (status != NOERROR)
    {
        current_task = previous;
        return ERROR;
    }

    return NOERROR;
}

int task_destroy(struct task_t *task)
{
    if (task == NULL)
        return ERROR;

    if (task == current_task)
        return ERROR;

    if (task->stack != NULL)
    {
        VALGRIND_STACK_DEREGISTER(task->vg_id);
        free(task->stack);
    }
    
    if (task->wait_queue != NULL)
        queue_destroy(task->wait_queue);

    if (task->name != NULL)
        free(task->name);

    free(task);

    return NOERROR;
}

void task_term()
{
    if (current_task != NULL)
    {
        free(current_task);
        current_task = NULL;

    }
}

void task_yield()
{
   current_task->status = TASK_READY;
   queue_add(ready_queue, current_task);
   task_switch(NULL);
}

void task_exit(int exit_code)
{
    current_task->exit_code = exit_code;
    current_task->status = TASK_TERMINATED;

    user_tasks--;

    task_switch(NULL);
}

static void task_suspend_locked(struct queue_t *queue)
{
    if (ready_queue != NULL)
        queue_del(ready_queue, current_task);

    current_task->status = TASK_SUSPENDED;
    current_task->suspended_queue = queue;

    if (queue != NULL)
        queue_add(queue, current_task);

}

void task_suspend(struct queue_t *queue)
{
    if (current_task == NULL)
        return;

    hw_irq_enable(0);
    task_suspend_locked(queue);
    hw_irq_enable(1);
    
    task_switch(NULL);
}

void task_awake(struct task_t *task)
{
    if (task == NULL)
        return;

    hw_irq_enable(0);

    if (task->status != TASK_SUSPENDED)
    {
        hw_irq_enable(1);
        return;
    }

    if (task->suspended_queue != NULL)
        queue_del(task->suspended_queue, task);

    task->suspended_queue = NULL;
    task->status = TASK_READY;

    queue_add(ready_queue, task);
    hw_irq_enable(1);
}

int task_wait(struct task_t *task)
{
    int exit_code;

    if (task == NULL || task == current_task)
        return ERROR;

    // protege a verificação e a entrada na fila de espera
    hw_irq_enable(0);

    // verifica se a tarefa já terminou
    if (task->status == TASK_TERMINATED)
    {
        exit_code = task->exit_code;
        hw_irq_enable(1);
        return exit_code;
    }

    // espera a tarefa terminar
    if (task->wait_queue == NULL)
    {
        hw_irq_enable(1);
        return ERROR;
    }

    task_suspend_locked(task->wait_queue);
    hw_irq_enable(1);
    task_switch(NULL);

    // continua aqui depois de ser acordada
    return task->exit_code;
}

void task_sleep(int t)
{
    if (t <= 0 || current_task == NULL || sleeping_queue == NULL)
        return;

    hw_irq_enable(0);

    current_task->wake_time = time() + t;
    task_suspend_locked(sleeping_queue);

    hw_irq_enable(1);

    task_switch(NULL);
}

void task_suspend_sem(struct queue_t *queue, int *lock)
{
    if (current_task == NULL)
        return;

    task_suspend_locked(queue);

    spin_unlock(lock);

    task_switch(NULL);
}

void task_awake_sem(struct task_t *task)
{
    if (task == NULL)
        return;

    if (task->status != TASK_SUSPENDED)
        return;

    if (task->suspended_queue != NULL)
        queue_del(task->suspended_queue, task);

    task->suspended_queue = NULL;
    task->status = TASK_READY;

    queue_add(ready_queue, task);
}