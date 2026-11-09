// GRR20234398 Henrico José Cornelio
// PingPongOS - PingPong Operating System

// Este arquivo PODE/DEVE ser alterado.

// Escalonador de tarefas prontas.

#include "scheduler.h"
#include "tcb.h"

extern struct task_t *task_current();

void sched_init()
{
}

void sched_term()
{
}

struct task_t *scheduler(struct queue_t *ready_queue)
{
    struct task_t *task;
    struct task_t *next;

    if (ready_queue == NULL)
        return NULL;

    next = NULL;
    
    task = queue_head(ready_queue);

    while (task != NULL)
    {
        // aplica aging
        task->dynamic_prio--;

        // escolhe a tarefa de maior prioridade
        if (next == NULL ||
            task->dynamic_prio < next->dynamic_prio)
        {
            next = task;
        }

        task = queue_next(ready_queue);
    }

    if (next != NULL)
    {
        // a tarefa escolhida volta à sua prioridade estática
        next->dynamic_prio = next->static_prio;
    }

    return next;
}


void sched_setprio(struct task_t *task, int prio)
{
    if (task == NULL)
        task = task_current();

    if (task == NULL) return;

    if (prio < -20)
        prio = -20;

    if (prio > 20)
        prio = 20;

    task->static_prio = prio;
    task->dynamic_prio = prio;
}

int sched_getprio(struct task_t *task)
{
    if (task == NULL)
        task = task_current();

    if (task == NULL)
        return 0;

    return task->static_prio;
}
