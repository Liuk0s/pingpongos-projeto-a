// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <stdio.h>
#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include "queue.h"		// biblioteca de filas genéricas

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
   struct task_t *prev, *next ;		// ponteiros para usar em filas
   int id ;				// identificador da tarefa
   ucontext_t context ;			// contexto armazenado da tarefa
   unsigned char state;  // indica o estado de uma tarefa (ver defines no final do arquivo ppos.h): 
                          // n - nova, r - pronta, x - executando, s - suspensa, e - terminada
   struct task_t* queue;
   struct task_t* joinQueue;
   int exitCode;
   unsigned int awakeTime; // used to store the time when it should be waked up

   void* custom_data; // internal data - do not modify!

   // ... (outros/novos campos deve ser adicionados APOS esse comentario)

    // Prioridades para escalonamento
   int prio_static;           // Prioridade estática da tarefa (-20 a +20)
   int prio_dynamic;          // Prioridade dinâmica (usada pelo escalonador)
   
   // Controle de preempção
   int quantum;               // Quantum de tempo restante (em ticks)
   
   // Métricas de contabilização
   unsigned int exec_start;   // Timestamp de quando a tarefa foi criada
   unsigned int proc_time;    // Tempo total de uso do processador (em ms)
   unsigned int last_proc;    // Timestamp da última vez que ganhou o processador
   unsigned int activations;  // Número de vezes que foi ativada
   unsigned int running_time; // Tempo de execução acumulado (em ticks)

   unsigned int user_task;    // Indica se é uma tarefa do sistema (0) ou de usuário 

} task_t ;

// estrutura que define um semáforo
typedef struct {
    struct task_t *queue;
    int value;

    unsigned char active;
} semaphore_t ;

// estrutura que define um mutex
typedef struct {
    struct task_t *queue;
    unsigned char value;

    unsigned char active;
} mutex_t ;

// estrutura que define uma barreira
typedef struct {
    struct task_t *queue;
    int maxTasks;
    int countTasks;
    unsigned char active;
    mutex_t mutex;
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct {
    void* content;
    int messageSize;
    int maxMessages;
    int countMessages;
    
    semaphore_t sBuffer;
    semaphore_t sItem;
    semaphore_t sVaga;
    
    unsigned char active;
} mqueue_t ;

#endif

