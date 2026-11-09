//GRR20234398 Henrico José Cornelio

// PingPongOS - PingPong Operating System
// © Prof. Carlos A. Maziero, DINF UFPR
// Versão 2.1 -- 06/2026

// Este arquivo PODE/DEVE ser alterado.

// Implementação do TAD fila genérica

#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

/*
 * Nó interno da fila.
 * Cada nó armazena um ponteiro genérico para o item
 * e um ponteiro para o próximo nó.
 */
struct node_t {
    void *item;
    struct node_t *next;
};

/*
 * Estrutura interna da fila.
 *
 * head     -> primeiro nó
 * tail     -> último nó
 * iterator -> nó atualmente apontado pelo iterador
 * size     -> quantidade de itens
 */
struct queue_t {
    struct node_t *head;
    struct node_t *tail;
    struct node_t *iterator;
    int size;
};


/*
 * Cria uma fila inicialmente vazia.
 */
struct queue_t *queue_create()
{
    struct queue_t *queue;

    queue = malloc(sizeof(struct queue_t));

    if (queue == NULL)
        return NULL;

    queue->head = NULL;
    queue->tail = NULL;
    queue->iterator = NULL;
    queue->size = 0;

    return queue;
}


/*
 * Destroi a fila.
 *
 * Os itens apontados pelos nós NÃO são liberados.
 * Apenas os nós e a própria fila são liberados.
 */
int queue_destroy(struct queue_t *queue)
{
    struct node_t *current;
    struct node_t *next;

    if (queue == NULL)
        return ERROR;

    current = queue->head;

    while (current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }

    free(queue);

    return NOERROR;
}


/*
 * Adiciona um item no final da fila.
 *
 * Se a fila estiver vazia, o iterador passa a apontar
 * para o primeiro item inserido.
 */
int queue_add(struct queue_t *queue, void *item)
{
    struct node_t *new_node;

    if (queue == NULL || item == NULL)
        return ERROR;

    new_node = malloc(sizeof(struct node_t));

    if (new_node == NULL)
        return ERROR;

    new_node->item = item;
    new_node->next = NULL;

    /*
     * Fila vazia.
     */
    if (queue->head == NULL)
    {
        queue->head = new_node;
        queue->tail = new_node;
        queue->iterator = new_node;
    }
    else
    {
        queue->tail->next = new_node;
        queue->tail = new_node;
    }

    queue->size++;

    return NOERROR;
}


/*
 * Remove a primeira ocorrência de item na fila.
 *
 * Se o nó removido for o nó apontado pelo iterador,
 * o iterador avança para o próximo nó.
 */
int queue_del(struct queue_t *queue, void *item)
{
    struct node_t *current;
    struct node_t *previous;

    if (queue == NULL || item == NULL)
        return ERROR;

    current = queue->head;
    previous = NULL;

    while (current != NULL)
    {
        if (current->item == item)
        {
            /*
             * Se o item removido é o item do iterador,
             * o iterador deve avançar.
             */
            if (queue->iterator == current)
                queue->iterator = current->next;

            /*
             * Removendo o primeiro nó.
             */
            if (previous == NULL)
                queue->head = current->next;
            else
                previous->next = current->next;

            /*
             * Removendo o último nó.
             */
            if (current == queue->tail)
                queue->tail = previous;

            free(current);
            queue->size--;

            /*
             * Caso a fila tenha ficado vazia.
             */
            if (queue->size == 0)
            {
                queue->head = NULL;
                queue->tail = NULL;
                queue->iterator = NULL;
            }

            return NOERROR;
        }

        previous = current;
        current = current->next;
    }

    return ERROR;
}


/*
 * Verifica se o item está presente na fila.
 */
bool queue_has(struct queue_t *queue, void *item)
{
    struct node_t *current;

    if (queue == NULL || item == NULL)
        return false;

    current = queue->head;

    while (current != NULL)
    {
        if (current->item == item)
            return true;

        current = current->next;
    }

    return false;
}


/*
 * Retorna o número de itens da fila.
 */
int queue_size(struct queue_t *queue)
{
    if (queue == NULL)
        return ERROR;

    return queue->size;
}


/*
 * Coloca o iterador no primeiro item da fila
 * e retorna esse item.
 */
void *queue_head(struct queue_t *queue)
{
    if (queue == NULL || queue->head == NULL)
        return NULL;

    queue->iterator = queue->head;

    return queue->iterator->item;
}


/*
 * Avança o iterador para o próximo item.
 */
void *queue_next(struct queue_t *queue)
{
    if (queue == NULL || queue->iterator == NULL)
        return NULL;

    queue->iterator = queue->iterator->next;

    if (queue->iterator == NULL)
        return NULL;

    return queue->iterator->item;
}


/*
 * Retorna o item atualmente apontado pelo iterador.
 */
void *queue_item(struct queue_t *queue)
{
    if (queue == NULL || queue->iterator == NULL)
        return NULL;

    return queue->iterator->item;
}


/*
 * Imprime os itens da fila usando a função fornecida
 * para imprimir cada item.
 */
void queue_print(char *name, struct queue_t *queue, void(func)(void *))
{
    struct node_t *current;

    if (name == NULL)
        name = "Queue";

    printf("%s: ", name);

    if (queue == NULL)
    {
        printf("undef\n");
        return;
    }

    printf("[ ");

    current = queue->head;

    while (current != NULL)
    {
        if (func != NULL)
            func(current->item);
        else
            printf("undef");

        if (current->next != NULL)
            printf(" ");

        current = current->next;
    }

    printf(" ] (%d items)\n", queue->size);
}


