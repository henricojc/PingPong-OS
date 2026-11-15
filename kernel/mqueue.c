//GRR20234398 Henrico José Cornelio

// PingPongOS - PingPong Operating System

// Este arquivo PODE/DEVE ser alterado.

// Gerência de filas de mensagens

#include <stdlib.h>
#include <string.h>

#include "mqueue.h"
#include "semaphore.h"
#include "map.h"

#define MQUEUE_MAP_SIZE 100

struct mqueue_t
{
    int id;

    unsigned char *buffer;

    int max_msgs;
    int msg_size;

    int head;
    int tail;
    int count;

    int mutex;
    int items;
    int slots;

    int destroyed;
    int refs;
};

static struct map_t *mqueue_map = NULL;


// Protege o mapa e a contagem de referências.

static int mqueue_lock = 0;


/*
 * Libera a memória da fila.
 * Só deve ser chamada quando não houver referências.
 */
static void mqueue_free(struct mqueue_t *queue)
{
    if (queue == NULL)
        return;

    free(queue->buffer);
    free(queue);
}


/*
 * Obtém uma referência para uma fila existente.
 */
static struct mqueue_t *mqueue_acquire(int id)
{
    struct mqueue_t *queue;

    spin_lock(&mqueue_lock);

    queue = map_get(mqueue_map, id);

    if (queue != NULL && !queue->destroyed)
        queue->refs++;
    else
        queue = NULL;

    spin_unlock(&mqueue_lock);

    return queue;
}


/*
 * Libera uma referência para a fila.
 */
static void mqueue_release(struct mqueue_t *queue)
{
    int should_free = 0;

    if (queue == NULL)
        return;

    spin_lock(&mqueue_lock);

    queue->refs--;

    if (queue->destroyed && queue->refs == 0)
        should_free = 1;

    spin_unlock(&mqueue_lock);

    if (should_free)
        mqueue_free(queue);
}


/*
 * Inicializa o subsistema.
 */
void mqueue_init()
{
    mqueue_lock = 0;
    mqueue_map = map_create(MQUEUE_MAP_SIZE);
}


/*
 * Encerra o subsistema.
 */
void mqueue_term()
{
    int i;

    if (mqueue_map == NULL)
        return;

    for (i = 0; i < map_size(mqueue_map); i++)
    {
        if (map_get(mqueue_map, i) != NULL)
            mqueue_destroy(i);
    }

    map_destroy(mqueue_map);
    mqueue_map = NULL;
}


/*
 * Cria uma fila de mensagens.
 */
int mqueue_create(int max_msgs, int msg_size)
{
    struct mqueue_t *queue;
    int id;

    if (mqueue_map == NULL || max_msgs <= 0 || msg_size <= 0)
        return ERROR;

    queue = malloc(sizeof(struct mqueue_t));

    if (queue == NULL)
        return ERROR;

    queue->buffer = malloc((size_t) max_msgs * (size_t) msg_size);

    if (queue->buffer == NULL)
    {
        free(queue);
        return ERROR;
    }

    queue->max_msgs = max_msgs;
    queue->msg_size = msg_size;

    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;

    queue->destroyed = 0;
    queue->refs = 0;

    queue->mutex = sem_create(1);
    queue->items = sem_create(0);
    queue->slots = sem_create(max_msgs);

    if (queue->mutex == ERROR ||
        queue->items == ERROR ||
        queue->slots == ERROR)
    {
        if (queue->mutex != ERROR)
            sem_destroy(queue->mutex);

        if (queue->items != ERROR)
            sem_destroy(queue->items);

        if (queue->slots != ERROR)
            sem_destroy(queue->slots);

        free(queue->buffer);
        free(queue);

        return ERROR;
    }

    spin_lock(&mqueue_lock);

    id = map_put(mqueue_map, queue);

    if (id != ERROR)
        queue->id = id;

    spin_unlock(&mqueue_lock);

    if (id == ERROR)
    {
        sem_destroy(queue->mutex);
        sem_destroy(queue->items);
        sem_destroy(queue->slots);

        free(queue->buffer);
        free(queue);

        return ERROR;
    }

    return id;
}


/*
 * Envia uma mensagem para a fila.
 */
int mqueue_send(int mqueue_id, void *msg)
{
    struct mqueue_t *queue;
    int result;

    if (msg == NULL)
        return ERROR;

    queue = mqueue_acquire(mqueue_id);

    if (queue == NULL)
        return ERROR;

    /*
     * Espera por uma posição livre.
     */
    if (sem_down(queue->slots) == ERROR)
    {
        mqueue_release(queue);
        return ERROR;
    }

    /*
     * Protege o buffer.
     */
    if (sem_down(queue->mutex) == ERROR)
    {
        mqueue_release(queue);
        return ERROR;
    }

    if (queue->destroyed)
    {
        sem_up(queue->mutex);
        mqueue_release(queue);
        return ERROR;
    }

    /*
     * Copia a mensagem para a posição de escrita.
     */
    memcpy(
        queue->buffer + ((size_t) queue->tail * queue->msg_size),
        msg,
        queue->msg_size
    );

    queue->tail = (queue->tail + 1) % queue->max_msgs;
    queue->count++;

    sem_up(queue->mutex);

    /*
     * Informa que existe uma mensagem disponível.
     */
    result = sem_up(queue->items);

    mqueue_release(queue);

    return result;
}


/*
 * Recebe uma mensagem da fila.
 */
int mqueue_recv(int mqueue_id, void *msg)
{
    struct mqueue_t *queue;
    int result;

    if (msg == NULL)
        return ERROR;

    queue = mqueue_acquire(mqueue_id);

    if (queue == NULL)
        return ERROR;

    /*
     * Espera por uma mensagem disponível.
     */
    if (sem_down(queue->items) == ERROR)
    {
        mqueue_release(queue);
        return ERROR;
    }

    /*
     * Protege o buffer.
     */
    if (sem_down(queue->mutex) == ERROR)
    {
        mqueue_release(queue);
        return ERROR;
    }

    if (queue->destroyed)
    {
        sem_up(queue->mutex);
        mqueue_release(queue);
        return ERROR;
    }

    /*
     * Copia a mensagem da posição de leitura.
     */
    memcpy(
        msg,
        queue->buffer + ((size_t) queue->head * queue->msg_size),
        queue->msg_size
    );

    queue->head = (queue->head + 1) % queue->max_msgs;
    queue->count--;

    sem_up(queue->mutex);

    /*
     * Informa que existe uma posição livre.
     */
    result = sem_up(queue->slots);

    mqueue_release(queue);

    return result;
}


/*
 * Destrói uma fila de mensagens.
 */
int mqueue_destroy(int mqueue_id)
{
    struct mqueue_t *queue;

    spin_lock(&mqueue_lock);

    queue = map_del(mqueue_map, mqueue_id);

    if (queue == NULL)
    {
        spin_unlock(&mqueue_lock);
        return ERROR;
    }

    /*
     * A referência da própria destruição impede
     * que a estrutura seja liberada prematuramente.
     */
    queue->refs++;
    queue->destroyed = 1;

    spin_unlock(&mqueue_lock);

    /*
     * Acorda tarefas esperando por vagas ou mensagens.
     */
    sem_destroy(queue->slots);
    sem_destroy(queue->items);

    /*
     * Aguarda operações que já entraram na região
     * crítica terminarem.
     */
    if (sem_down(queue->mutex) == NOERROR)
        sem_destroy(queue->mutex);

    /*
     * Libera a referência da destruição.
     * A memória será liberada quando todas as operações
     * que já tinham adquirido a fila terminarem.
     */
    mqueue_release(queue);

    return NOERROR;
}


/*
 * Retorna a quantidade de mensagens na fila.
 */
int mqueue_msgs(int mqueue_id)
{
    struct mqueue_t *queue;
    int count;

    queue = mqueue_acquire(mqueue_id);

    if (queue == NULL)
        return ERROR;

    if (sem_down(queue->mutex) == ERROR)
    {
        mqueue_release(queue);
        return ERROR;
    }

    if (queue->destroyed)
    {
        sem_up(queue->mutex);
        mqueue_release(queue);
        return ERROR;
    }

    count = queue->count;

    sem_up(queue->mutex);

    mqueue_release(queue);

    return count;
}
