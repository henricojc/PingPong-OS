// GRR20234398 Henrico José Cornelio
// PingPongOS - PingPong Operating System

// Este arquivo PODE/DEVE ser alterado.

// Gerência básica do tempo.
#include <stddef.h>
#include "../hardware/cpu.h"
#include "task.h"
#include "tcb.h"


static volatile unsigned int system_time = 0;
extern struct task_t *task_current();

static void timer_handler(int signum)
{
    struct task_t *task;

    system_time++;

    task = task_current();

    if (task == NULL)
        return;

    if (!task->is_user)
        return;

    if (task->status != TASK_RUNNING)
        return;

    task->quantum--;

    if (task->quantum <= 0)
        task_yield();

    if (!task->is_user || task->status != TASK_RUNNING)
        return;
}

void time_init()
{
    system_time = 0;
    hw_irq_handle(IRQ_TIMER, timer_handler);
    hw_timer(1, 1);
}

void time_term()
{
    hw_timer(0, 0);
    hw_irq_handle(IRQ_TIMER, NULL);
}

unsigned int time()
{
    return system_time;
}

