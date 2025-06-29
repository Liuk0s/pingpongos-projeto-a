/**
 * ============================================================================
 * PingPongOS - Projeto A
 * Implementação do Escalonador por Prioridades, Preempção e Contabilização
 * 
 * Alunos: Iaritzza Bielinki
 *         Lucas Giovanni Thuler
 * 
 * Implementa escalonador preemptivo com aging, sistema de quantum e
 * contabilização de tempo de CPU por tarefa.
 * ============================================================================
 */

#define _XOPEN_SOURCE 600           

#include <signal.h>
#include <sys/time.h>
#include "ppos.h"
#include "ppos-core-globals.h"
#include "ppos-disk-manager.h"

/**
 * ============================================================================
 * CONSTANTES E CONFIGURAÇÕES DO SISTEMA
 * ============================================================================
 */

// Configuração do escalonador por prioridades
#define PRIORITY_ALPHA     -1   // Aging: decrementa prioridade das não-escolhidas
#define PRIORITY_MAX      -20   // Maior prioridade (números menores = mais prioritário)
#define PRIORITY_DEF        0   // Prioridade padrão para novas tarefas
#define PRIORITY_MIN       20   // Menor prioridade
// Operadores para comparação de prioridades
#define MAIS_PRIO           <
#define MENOS_PRIO          >

// Configuração do sistema de tempo
#define QUANTUM_SIZE       20   // Quantum em ticks (20ms)
#define TIMER_INTERVAL   1000   // Timer dispara a cada 1ms


// Variáveis globais do sistema

unsigned int _systemTime;               // Relógio global em milissegundos
static struct sigaction timer_action;   // Handler para SIGALRM
static struct itimerval timer;          // Configuração do timer UNIX

// Protótipos das funções auxiliares
static void interrupt_handler(int signum);
static void timer_init(void);

/**
 * ============================================================================
 * ESCALONADOR
 * ============================================================================
 */

/**
 * Escalonador por prioridades com aging
 * 
 * Política: menor valor = maior prioridade, aging diminui valores das 
 * não-escolhidas, desempate por ID (FIFO). Apenas tarefas de usuário 
 * participam do aging.
 */
task_t* scheduler() {

    PPOS_PREEMPT_DISABLE;   // Protege estruturas do escalonador
    
    if (readyQueue == NULL) {
        PPOS_PREEMPT_ENABLE;
        return NULL;
    }

    // Busca tarefa com melhor prioridade dinâmica
    task_t* better  = readyQueue;
    task_t* current = readyQueue;

    #ifdef DEBUG02
    printf("\n[SCHED] Tarefas prontas:");
    do {
        printf("\nT%d(prio=%d)", current->id, current->prio_dynamic);
        current = current->next;
    } while (current != readyQueue);
    printf("\n");
    #endif

    // Encontra melhor tarefa: menor prioridade dinâmica, menor ID em empates
    do {
        if ((current->prio_dynamic  < better->prio_dynamic) || 
            (current->prio_dynamic == better->prio_dynamic  &&
             current->id < better->id))
        {
            better = current;
        }
        current = current->next;
    }
    while (current != readyQueue);

    #ifdef DEBUG02
    printf("→ Escolhida: T%d\n", better->id);
    #endif

    // Aging: apenas para tarefas de usuário (user_task = 1)
    if (better->user_task) {
        current = readyQueue;
        do {
            // Aplica aging nas tarefas não-escolhidas
            if (current != better) {
                if (current->prio_dynamic MENOS_PRIO PRIORITY_MAX) {
                    current->prio_dynamic += PRIORITY_ALPHA; // Melhora prioridade
                }
                // Limita ao valor máximo de prioridade
                if (current->prio_dynamic MAIS_PRIO PRIORITY_MAX) {
                    current->prio_dynamic = PRIORITY_MAX;
                }
            }
            current = current->next;
        }
        while (current != readyQueue);

        // Reset: tarefa escolhida volta à prioridade original
        better->prio_dynamic = better->prio_static;
    }

    PPOS_PREEMPT_ENABLE;
    return better;
}

/**
 * ============================================================================
 * FUNÇÕES DE TEMPO E PRIORIDADE
 * ============================================================================
 */

unsigned int systime() {
    return _systemTime;
}

void task_setprio(task_t *task, int prio) {
    if (task == NULL) {
        task = taskExec;
    }
    
    // Clamp nos limites válidos
    if (prio MAIS_PRIO PRIORITY_MAX)  prio = PRIORITY_MAX;
    if (prio MENOS_PRIO PRIORITY_MIN) prio = PRIORITY_MIN;
    
    task->prio_static  = prio;
    task->prio_dynamic = prio;
}

int task_getprio(task_t *task) {
    if (task == NULL) {
        task = taskExec;
    }

    return task->prio_static;
}

/**
 * ============================================================================
 * SISTEMA DE PREEMPÇÃO POR TEMPO
 * ============================================================================
 */

/**
 * Handler do timer UNIX - simula tick do hardware
 * Incrementa relógio global e controla quantum das tarefas de usuário
 */
void interrupt_handler(int signum) {
    _systemTime++;

    #ifdef DEBUG02
        printf("\n[DEBUG02] Timer tick %d, task %d, quantum %d", systime(), taskExec->id, taskExec->quantum);
    #endif

    // Controle de quantum apenas para tarefas de usuário
    if (!taskExec->user_task) {
        return;
    }

    taskExec->quantum--;

    // Preempção por esgotamento de quantum
    if (taskExec->quantum <= 0 && PPOS_IS_PREEMPT_ACTIVE) { 
        task_yield();
    }
}

/**
 * Inicializa timer UNIX que simula clock do sistema
 */
void timer_init() {
    // Configura handler do sinal SIGALRM
    timer_action.sa_handler = interrupt_handler; 
    sigemptyset(&timer_action.sa_mask);           
    timer_action.sa_flags   = 0;            
    
    if (sigaction(SIGALRM, &timer_action, 0) < 0) {
        perror("Erro em sigaction: ");
        exit(1);
    }
    
    // Timer periódico de 1ms
    timer.it_value.tv_usec    = TIMER_INTERVAL;
    timer.it_value.tv_sec     = 0;
    timer.it_interval.tv_usec = TIMER_INTERVAL;
    timer.it_interval.tv_sec  = 0;
    
    if (setitimer(ITIMER_REAL, &timer, 0) < 0) {
        perror("Erro em setitimer: ");
        exit(1);
    }
}

/**
 * ============================================================================
 * HOOKS DO SISTEMA
 * ============================================================================
 */

void before_ppos_init () {
#ifdef DEBUG
    printf("\ninit - BEFORE");
#endif    
}

/**
 * Inicialização completa do sistema: timer, configurações e preempção
 */
void after_ppos_init () {
#ifdef DEBUG
    printf("\ninit - AFTER");
#endif

    _systemTime = 0;
    timer_init();

    // Main é tarefa de sistema (não sofre preempção por quantum)
    taskMain->user_task = 0;

    PPOS_PREEMPT_ENABLE;
}

/**
 * ============================================================================
 * HOOKS DE GERENCIAMENTO DE TAREFAS
 * ============================================================================
 */


void before_task_create (task_t *task ) {
#ifdef DEBUG
    printf("\ntask_create - BEFORE - [%d]", task->id);
#endif

    PPOS_PREEMPT_DISABLE;   // Criação de tarefa é área crítica
}

/**
 * Inicializa todos os campos específicos da nova tarefa
 */
void after_task_create (task_t *task ) {
#ifdef DEBUG
    printf("\ntask_create - AFTER - [%d]", task->id);
#endif

    // Configuração inicial de prioridades e quantum
    task->prio_static  = PRIORITY_DEF;
    task->prio_dynamic = PRIORITY_DEF;
    task->quantum      = QUANTUM_SIZE;

    // Classificação: id > 1 = usuário, id <= 1 = sistema
    task->user_task    = (task->id > 1) ? 1 : 0;

    // Inicialização das métricas de contabilização
    task->exec_start   = systime();     
    task->proc_time    = 0;             
    task->last_proc    = 0;            
    task->activations  = 0;             
    task->running_time = 0;             
    
    PPOS_PREEMPT_ENABLE;
}

void before_task_exit () {
#ifdef DEBUG
    printf("\ntask_exit - BEFORE - [%d]", taskExec->id);
#endif

    PPOS_PREEMPT_DISABLE;

}

/**
 * Calcula e exibe estatísticas finais da tarefa
 */
void after_task_exit () {
#ifdef DEBUG
    printf("\ntask_exit - AFTER- [%d]", taskExec->id);
#endif

    unsigned int task_total_time = systime() - taskExec->exec_start;

    printf("Task %d exit: execution time %u ms, processor time %u ms, %u activations\n",
        taskExec->id,           
        task_total_time,        
        taskExec->proc_time,    
        taskExec->activations);
}

//Se a main (task 0) terminou, sinaliza disk manager para encerrar
    if (taskExec->id == 0) {
        extern void disk_mgr_shutdown();  // Declara função do ppos_disk.c
        disk_mgr_shutdown();
    }

/**
 * Contabiliza tempo da tarefa que está saindo do processador
 */
void before_task_switch ( task_t *task ) {
#ifdef DEBUG
    printf("\ntask_switch - BEFORE - [%d -> %d]", taskExec->id, task->id);
#endif

    PPOS_PREEMPT_DISABLE;

    // Atualiza tempo de processador da tarefa atual
    if (taskExec && taskExec->user_task && taskExec->last_proc > 0) {
        unsigned int time_slice = systime() - taskExec->last_proc;
        taskExec->proc_time += time_slice;
    }
}

/**
 * Inicializa contadores da tarefa que está entrando no processador
 */
void after_task_switch ( task_t *task ) {
#ifdef DEBUG
    printf("\ntask_switch - AFTER - [%d -> %d]", taskExec->id, task->id);
#endif

    if (task && task->user_task) {
        task->activations++;
        task->last_proc = systime();
        task->quantum   = QUANTUM_SIZE;     // Quantum completo para nova tarefa
    }

    PPOS_PREEMPT_ENABLE;
}

void before_task_yield () {
#ifdef DEBUG02
    printf("\ntask_yield - BEFORE - [%d]", taskExec->id);
#endif

    PPOS_PREEMPT_DISABLE;

}

void after_task_yield () {
#ifdef DEBUG02
    printf("\ntask_yield - AFTER - [%d]", taskExec->id);
#endif
    // Preempção reabilitada em after_task_switch
}

void before_task_suspend( task_t *task ) {
#ifdef DEBUG
    printf("\ntask_suspend - BEFORE - [%d]", task->id);
#endif

    PPOS_PREEMPT_DISABLE;
}

void after_task_suspend( task_t *task ) {

#ifdef DEBUG
    printf("\ntask_suspend - AFTER - [%d]", task->id);
#endif

    PPOS_PREEMPT_ENABLE;
}

void before_task_resume(task_t *task) {
#ifdef DEBUG
    printf("\ntask_resume - BEFORE - [%d]", task->id);
#endif

    PPOS_PREEMPT_DISABLE;
}

void after_task_resume(task_t *task) {
#ifdef DEBUG
    printf("\ntask_resume - AFTER - [%d]", task->id);
#endif

    PPOS_PREEMPT_ENABLE;
}

void before_task_sleep () {
#ifdef DEBUG
    printf("\ntask_sleep - BEFORE - [%d]", taskExec->id);
#endif

    PPOS_PREEMPT_DISABLE;    
}

void after_task_sleep () {
#ifdef DEBUG
    printf("\ntask_sleep - AFTER - [%d]", taskExec->id);
#endif

    // Preempção reabilitada em after_task_switch
}

int before_task_join (task_t *task) {
#ifdef DEBUG
    printf("\ntask_join - BEFORE - [%d]", taskExec->id);
#endif

    PPOS_PREEMPT_DISABLE;

    return 0;
}

int after_task_join (task_t *task) {
#ifdef DEBUG
    printf("\ntask_join - AFTER - [%d]", taskExec->id);
#endif

    PPOS_PREEMPT_ENABLE;

    return 0;
}


/**
 * ============================================================================
 * SEÇÃO 8: HOOKS DE SEMÁFOROS, MUTEXES, BARREIRAS E FILAS
 * (Não necessário para a parte A)
 * ============================================================================
 */


int before_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}
