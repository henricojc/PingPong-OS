//GRR20234398 Henrico José Cornelio

// PingPongOS - PingPong Operating System

// Este arquivo PODE/DEVE ser alterado.

// Dispatcher: gerencia os estados das tarefas.
#include <stddef.h>
#include "dispatcher.h"
#include "scheduler.h"
#include "task.h"
#include "lib/pplibc.h"

extern void user_main(void *arg);
extern unsigned int time();
extern struct task_t *task_kernel;

struct queue_t *ready_queue = NULL;
struct queue_t *sleeping_queue;
static struct queue_t *terminated_queue = NULL;
int user_tasks = 0;

void dispatcher_init()
{
    ready_queue = queue_create();
    terminated_queue = queue_create();
    sleeping_queue = queue_create();

    if (ready_queue == NULL ||
        terminated_queue == NULL ||
        sleeping_queue == NULL)
    {
        printk("Erro ao criar filas do dispatcher\n");
    }
}

void dispatcher_term()
{
    if (ready_queue != NULL)
    {
        queue_destroy(ready_queue);
        ready_queue = NULL;
    }

    if (sleeping_queue != NULL)
    {
        queue_destroy(sleeping_queue);
        sleeping_queue = NULL;
    }
}

static void wake_waiters(struct task_t *task)
{
    struct task_t *waiter;
    int size_before;

    if (task == NULL || task->wait_queue == NULL)
        return;

    while ((waiter = queue_head(task->wait_queue)) != NULL)
    {
        size_before = queue_size(task->wait_queue);
        task_awake(waiter);

        // evita loop infinito caso task_awake não remova o elemento
        if (queue_size(task->wait_queue) >= size_before)
        {
            printk("ERRO: esperador nao foi removido da fila\n");
            break;
        }
    }
}

void check_sleeping_tasks()
{
    struct task_t *task;
    struct task_t *next;
    unsigned int current_time;

    if (sleeping_queue == NULL)
        return;

    current_time = time();

    task = queue_head(sleeping_queue);

    while (task != NULL)
    {
        // avança o iterador antes de eventualmente remover a tarefa atual
        next = queue_next(sleeping_queue);

        if (current_time >= task->wake_time)
        {
            task_awake(task);
        }

        task = next;
    }
}

void dispatcher()
{
    struct task_t *task_user;
    struct task_t *next;
    struct task_t *finished;
    
    // cria a tarefa que executará user_main()
    task_user = task_create("user_main", user_main, NULL);

    if (task_user == NULL)
        return;

    while (user_tasks > 0)
    {
        check_sleeping_tasks();
        next = scheduler(ready_queue);

        if (next != NULL)
        {
            task_run(next);
            if (next->status == TASK_TERMINATED)
            {
                wake_waiters(next);
                printk("PPOS: task %3d (%s), %5u ms run, %5u ms cpu, %5u acts, exit code %3d\n",
                    next->id,
                    next->name,
                    time() - next->creation_time,
                    next->cpu_time,
                    next->activations,
                    next->exit_code);
            }
        }
    }

    // contabilização final do kernel
    task_kernel->cpu_time += time() - task_kernel->last_cpu_start;

    printk("PPOS: task %3d (%s), %5u ms run, %5u ms cpu, %5u acts, exit code %3d\n",
        task_kernel->id,
        task_kernel->name,
        time() - task_kernel->creation_time,
        task_kernel->cpu_time,
        task_kernel->activations,
        task_kernel->exit_code);
    
    while (queue_size(terminated_queue) > 0)
    {
        finished = queue_head(terminated_queue);
        queue_del(terminated_queue, finished);

        task_destroy(finished);
    }

    // quando user retornar ao kernel, a tarefa pode ser destruída
    task_destroy(task_user);
}

void task_run(struct task_t *task)
{
    if (task == NULL)
        return;

    queue_del(ready_queue, task);
    
    task->status = TASK_RUNNING;
    task->quantum = 10;

    task->last_cpu_start = time();

    task_switch(task);
}

