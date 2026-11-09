//GRR20234398 Henrico José Cornelio

// PingPongOS - PingPong Operating System

// Este arquivo PODE/DEVE ser alterado.

// Semáforos e spinlocks

#include <stdlib.h>
#include "semaphore.h"
#include "queue.h"
#include "map.h"
#include "task.h"
#include "hardware/cpu.h"

extern void task_suspend_sem(struct queue_t *queue, int *lock);
extern void task_awake_sem(struct task_t *task);

struct semaphore_t
{
    int id;
    int counter;
    struct queue_t *waiting;
    int lock;
    int destroyed;
    int pending;
};
static struct map_t *semaphore_map = NULL;

void sem_init()
{
    semaphore_map = map_create(100);
}

int sem_create(int value)
{
    struct semaphore_t *sem;
    int id;

    if (value < 0 || semaphore_map == NULL)
        return ERROR;

    sem = malloc(sizeof(struct semaphore_t));

    if (sem == NULL)
        return ERROR;

    sem->counter = value;
    sem->lock = 0;
    sem->waiting = queue_create();
    sem->destroyed = 0;
    sem->pending = 0;

    if (sem->waiting == NULL)
    {
        free(sem);
        return ERROR;
    }

    id = map_put(semaphore_map, sem);

    if (id == ERROR)
    {
        queue_destroy(sem->waiting);
        free(sem);
        return ERROR;
    }

    sem->id = id;

    return id;
}

void sem_term()
{
    if (semaphore_map != NULL)
    {
        map_destroy(semaphore_map);
        semaphore_map = NULL;
    }
}

void spin_lock(int *lock)
{
    hw_irq_enable(0);

    while (__sync_lock_test_and_set(lock, 1));
}

void spin_unlock(int *lock)
{
    __sync_lock_release(lock);

    hw_irq_enable(1);
}

int sem_down(int sem_id)
{
    struct semaphore_t *sem;
    int destroy;

    sem = map_get(semaphore_map, sem_id);

    if (sem == NULL)
        return ERROR;

    spin_lock(&sem->lock);

    if (sem->destroyed)
    {
        spin_unlock(&sem->lock);
        return ERROR;
    }

    sem->counter--;

    if (sem->counter >= 0)
    {
        spin_unlock(&sem->lock);
        return NOERROR;
    }

    sem->pending++;

    task_suspend_sem(sem->waiting, &sem->lock);

    // A tarefa foi acordada. Verifica por que isso aconteceu.
    spin_lock(&sem->lock);

    destroy = sem->destroyed;

    if (destroy)
        sem->pending--;

    spin_unlock(&sem->lock);

    if (destroy)
    {
        if (sem->pending == 0)
        {
            queue_destroy(sem->waiting);
            free(sem);
        }

        return ERROR;
    }

    return NOERROR;
}

int sem_up(int sem_id)
{
    struct semaphore_t *sem;
    struct task_t *task;

    sem = map_get(semaphore_map, sem_id);

    if (sem == NULL)
        return ERROR;

    spin_lock(&sem->lock);

    sem->counter++;

    if (sem->counter <= 0)
    {
        task = queue_head(sem->waiting);

        if (task != NULL)
            task_awake_sem(task);
    }

    spin_unlock(&sem->lock);

    return NOERROR;
}

int sem_destroy(int sem_id)
{
    struct semaphore_t *sem;
    struct task_t *task;

    sem = map_get(semaphore_map, sem_id);

    if (sem == NULL)
        return ERROR;

    spin_lock(&sem->lock);

    sem = map_del(semaphore_map, sem_id);

    if (sem == NULL)
    {
        spin_unlock(&sem->lock);
        return ERROR;
    }

    sem->destroyed = 1;

    // Acorda todas as tarefas bloqueadas.
    while ((task = queue_head(sem->waiting)) != NULL)
        task_awake_sem(task);

    // Se não houver tarefas pendentes, podemos liberar tudo.
    if (sem->pending == 0)
    {
        spin_unlock(&sem->lock);

        queue_destroy(sem->waiting);
        free(sem);
    }
    else
    {
        spin_unlock(&sem->lock);
    }

    return NOERROR;
}