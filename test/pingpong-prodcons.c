
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ppos.h"
#include "semaphore.h"

#define BUFFER_SIZE 5
#define NUM_PRODUCERS 3
#define NUM_CONSUMERS 2

// Buffer circular FIFO
static int buffer[BUFFER_SIZE];
static int buffer_in = 0;
static int buffer_out = 0;

// Semáforos
static int s_buffer;
static int s_item;
static int s_vaga;

// Estrutura para identificar cada tarefa
struct task_args
{
    int id;
};

// Argumentos e estruturas das tarefas
static struct task_args producer_args[NUM_PRODUCERS] = {
    {1}, {2}, {3}
};

static struct task_args consumer_args[NUM_CONSUMERS] = {
    {1}, {2}
};

// --------------------------------------------------
// PRODUTOR
// --------------------------------------------------

void produtor(void *arg)
{
    struct task_args *args = (struct task_args *)arg;
    int item;

    while (1)
    {
        // Simula o tempo necessário para produzir
        task_sleep(1000);

        // Gera um número entre 0 e 99
        item = rand() % 100;

        // Aguarda uma posição livre no buffer
        if (sem_down(s_vaga) != 0)
            break;

        // Solicita acesso exclusivo ao buffer
        if (sem_down(s_buffer) != 0)
        {
            sem_up(s_vaga);
            break;
        }

        // Insere o item no buffer
        buffer[buffer_in] = item;
        buffer_in = (buffer_in + 1) % BUFFER_SIZE;

        // Imprime enquanto mantém o acesso exclusivo.
        // Isso preserva a ordem de produção na saída.
        printk("p%d produziu %d\n", args->id, item);
        // Libera o buffer
        sem_up(s_buffer);

        // Informa que existe um novo item disponível
        sem_up(s_item);
    }
}


// --------------------------------------------------
// CONSUMIDOR
// --------------------------------------------------

void consumidor(void *arg)
{
    struct task_args *args = (struct task_args *)arg;
    int item;

    while (1)
    {
        // Aguarda a existência de um item disponível
        if (sem_down(s_item) != 0)
            break;

        // Solicita acesso exclusivo ao buffer
        if (sem_down(s_buffer) != 0)
        {
            sem_up(s_item);
            break;
        }

        // Retira o item mais antigo do buffer
        item = buffer[buffer_out];
        buffer_out = (buffer_out + 1) % BUFFER_SIZE;

        // Imprime dentro da região crítica para preservar
        // a ordem FIFO também na saída do programa.
        printk("\t\t\t\t\tc%d consumiu %d\n", args->id, item);

        // Libera o buffer
        sem_up(s_buffer);

        // Informa que existe uma posição livre
        sem_up(s_vaga);

        // Simula o tempo necessário para consumir
        task_sleep(1000);
    }
}


// --------------------------------------------------
// PROGRAMA PRINCIPAL
// --------------------------------------------------

void user_main(void)
{
    int i;

    // Inicializa o gerador de números aleatórios
    srand(time(NULL));

    // Cria os semáforos
    s_buffer = sem_create(1);
    s_item = sem_create(0);
    s_vaga = sem_create(BUFFER_SIZE);

    if (s_buffer < 0 || s_item < 0 || s_vaga < 0)
    {
        printk("Erro ao criar os semaforos\n");
        task_exit(1);
    }

    // Cria os três produtores
    for (i = 0; i < NUM_PRODUCERS; i++)
    {
        char name[20];

        sprintf(name, "produtor-%d", i + 1);

        if (task_create(name, produtor, &producer_args[i]) == NULL)
        {
            printk("Erro ao criar produtor %d\n", i + 1);
            task_exit(1);
        }
    }

    // Cria os dois consumidores
    for (i = 0; i < NUM_CONSUMERS; i++)
    {
        char name[20];

        sprintf(name, "consumidor-%d", i + 1);

        if (task_create(name, consumidor, &consumer_args[i]) == NULL)
        {
            printk("Erro ao criar consumidor %d\n", i + 1);
            task_exit(1);
        }
    }

    // Encerra a tarefa principal.
    // Produtores e consumidores continuam executando.
    task_exit(0);
}
