//GRR20234398 Henrico José Cornelio

// PingPongOS - PingPong Operating System
// © Prof. Carlos A. Maziero, DINF UFPR
// Versão 2.1 -- 06/2026

// Este arquivo PODE/DEVE ser alterado.

// Implementação do TAD Mapa de objetos

// PingPongOS - PingPong Operating System
// Implementação do TAD mapa

#include <stdio.h>
#include <stdlib.h>
#include "map.h"


/*
 * Estrutura interna do mapa.
 *
 * objects -> vetor de ponteiros para os objetos
 * size    -> capacidade máxima do mapa
 * items   -> quantidade atual de objetos registrados
 * next_id -> próximo ID a partir do qual devemos procurar
 */
struct map_t
{
    void **objects;
    int size;
    int items;
    int next_id;
};


/*
 * Cria um mapa para "size" objetos.
 *
 * Os IDs possíveis serão:
 *
 * 0, 1, 2, ..., size - 1
 */
struct map_t *map_create(int size)
{
    struct map_t *map;

    if (size <= 0)
        return NULL;

    map = malloc(sizeof(struct map_t));

    if (map == NULL)
        return NULL;

    map->objects = calloc(size, sizeof(void *));

    if (map->objects == NULL)
    {
        free(map);
        return NULL;
    }

    map->size = size;
    map->items = 0;
    map->next_id = 0;

    return map;
}


/*
 * Destrói o mapa.
 *
 * Os objetos armazenados NÃO são liberados.
 */
int map_destroy(struct map_t *map)
{
    if (map == NULL)
        return ERROR;

    free(map->objects);
    free(map);

    return NOERROR;
}


/*
 * Registra um objeto no próximo ID livre.
 *
 * A busca começa em next_id e percorre o mapa
 * de forma circular.
 */
int map_put(struct map_t *map, void *object)
{
    int id;
    int i;

    if (map == NULL || object == NULL)
        return ERROR;

    /*
     * Se o mapa estiver cheio, não há ID disponível.
     */
    if (map->items == map->size)
        return ERROR;

    /*
     * Procura um ID livre começando em next_id.
     */
    for (i = 0; i < map->size; i++)
    {
        id = (map->next_id + i) % map->size;

        if (map->objects[id] == NULL)
        {
            map->objects[id] = object;
            map->items++;

            /*
             * O próximo put() começará procurando
             * a partir do ID seguinte.
             */
            map->next_id = (id + 1) % map->size;

            return id;
        }
    }

    return ERROR;
}


/*
 * Retorna o objeto associado ao ID.
 */
void *map_get(struct map_t *map, int id)
{
    if (map == NULL)
        return NULL;

    if (id < 0 || id >= map->size)
        return NULL;

    return map->objects[id];
}


/*
 * Remove o objeto associado ao ID e retorna o objeto.
 */
void *map_del(struct map_t *map, int id)
{
    void *object;

    if (map == NULL)
        return NULL;

    if (id < 0 || id >= map->size)
        return NULL;

    /*
     * Não existe objeto nesse ID.
     */
    if (map->objects[id] == NULL)
        return NULL;

    object = map->objects[id];

    map->objects[id] = NULL;
    map->items--;

    /*
     * Se o ID liberado for anterior ao próximo ID,
     * ele pode ser reutilizado antes dos IDs posteriores.
     *
     * Isso mantém a busca pelos IDs livres crescente,
     * considerando o comportamento circular.
     */
    if (id < map->next_id)
        map->next_id = id;

    return object;
}


/*
 * Retorna a quantidade de objetos atualmente registrados.
 */
int map_items(struct map_t *map)
{
    if (map == NULL)
        return ERROR;

    return map->items;
}


/*
 * Retorna a capacidade máxima do mapa.
 */
int map_size(struct map_t *map)
{
    if (map == NULL)
        return ERROR;

    return map->size;
}


/*
 * Imprime o conteúdo do mapa.
 */
void map_print(char *name, struct map_t *map)
{
    int i;

    if (name == NULL)
        name = "Mapa";

    printf("%s: ", name);

    if (map == NULL)
    {
        printf("undef\n");
        return;
    }

    printf("[ ");

    for (i = 0; i < map->size; i++)
    {
        if (map->objects[i] == NULL)
            printf("-");
        else
            printf("*");

        if (i < map->size - 1)
            printf(" ");
    }

    printf(" ] (%d/%d)\n", map->items, map->size);
}