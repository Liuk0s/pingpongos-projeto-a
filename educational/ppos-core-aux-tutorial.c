/**
 * ============================================================================
 * PingPongOS - Projeto A
 * Implementação do Escalonador por Prioridades, Preempção e Contabilização
 * 
 * Alunos: Iaritzza Bielinki
 *         Lucas Giovanni Thuler
 * 
 * Este arquivo contém as implementações necessárias para:
 * 1. Escalonador preemptivo baseado em prioridades com envelhecimento (aging)
 * 2. Sistema de preempção por tempo usando quantums e timer UNIX
 * 3. Contabilização de uso do processador por cada tarefa
 * ============================================================================
 */

// Define compatibilidade POSIX para usar funções de timer e sinais
#define _XOPEN_SOURCE 600           

#include <signal.h>     // Para manipulação de sinais UNIX (SIGALRM)
#include <sys/time.h>   // Para funções de timer (setitimer, getitimer)

#include "ppos.h"
#include "ppos-core-globals.h"
#include "ppos-disk-manager.h"


// ****************************************************************************
// Adicione TUDO O QUE FOR NECESSARIO para realizar o seu trabalho
// Coloque as suas modificações aqui, 
// p.ex. includes, defines variáveis, 
// estruturas e funções
//
// ****************************************************************************

/**
 * =========================================================================
 * SEÇÃO 1: CONSTANTES E CONFIGURAÇÕES DO SISTEMA                           
 * =========================================================================
 */

/* ===== CONSTANTES DE PRIORIDADES ===== */
#define PRIORITY_ALPHA     -1   // Fator de envelhecimento: -1 significa que a cada 
                                // ciclo do scheduler, tarefas não-escolhidas ganham 
                                // +1 de prioridade (ficam "mais prioritárias")
#define PRIORITY_MAX      -20   // Maior prioridade (valores baixos = alta prioridade)
#define PRIORITY_DEF        0   // Prioridade padrão para novas tarefas
#define PRIORITY_MIN       20   // Menor prioridade (valores altos = baixa prioridade)
#define MAIS_PRIO           <   // Para lidar alterações nos valores de prioridade
#define MENOS_PRIO          >   // Para lidar alterações nos valores de prioridade

/* ===== CONSTANTES DE TEMPORIZAÇÃO ===== */
#define QUANTUM_SIZE       20   // Quantum em ticks do timer
#define TIMER_INTERVAL   1000   // Intervalo do timer em microsegundos (1ms)


/* ===== VARIÁVEIS GLOBAIS DO PROJETO ===== */

// Relógio global do sistema - conta o tempo em milissegundos desde a inicialização
unsigned int _systemTime;

// Estruturas para controle do timer UNIX que simula o clock do hardware
static struct sigaction timer_action;    // Define o que fazer quando receber SIGALRM
static struct itimerval timer;           // Configura os intervalos do timer

// Declarações de funções auxiliares
static void interrupt_handler(int signum);
static void timer_init(void);

/**
 * ============================================================================
 * SEÇÃO 2: ESCALONADOR (SCHEDULER)
 * ============================================================================
 */

 /**
 * Função do escalonador - escolhe a próxima tarefa a executar
 * 
 * ALGORITMO:
 * 1. Procura a tarefa com MENOR valor de prioridade dinâmica
 * 2. Em caso de empate, escolhe a de menor ID (FIFO)
 * 3. Aplica aging nas tarefas não-escolhidas (só para tarefas de usuário)
 * 4. Reseta a prioridade dinâmica da tarefa escolhida
 * 
 * @return Ponteiro para a tarefa escolhida ou NULL se fila vazia
 */
task_t* scheduler() {

    // INÍCIO DA ÁREA CRÍTICA
    // Desabilita preempção para evitar que o timer interrompa o escalonador
    PPOS_PREEMPT_DISABLE; 
    
    // Se não há tarefas prontas, retorna NULL
    if (readyQueue == NULL) {
        PPOS_PREEMPT_ENABLE;        // Sempre reabilitar antes de retornar!
        return NULL;
    }

    // Inicializa ponteiros para percorrer a fila circular de tarefas prontas
    task_t* better  = readyQueue;   // Melhor tarefa encontrada até agora
    task_t* current = readyQueue;   // Tarefa atual sendo analisada

    #ifdef DEBUG02
    // Debug: mostra todas as tarefas prontas e suas prioridades
    printf("\n[SCHED] Tarefas prontas:");
    do {
        printf("\nT%d(prio=%d)", current->id, current->prio_dynamic);
        current = current->next;
    } while (current != readyQueue);
    printf("\n");
    #endif

    // PASSO 1: Encontra a melhor tarefa usando política de prioridades
    do {
        // Critério de escolha:
        // 1º - Menor prioridade dinâmica (valores menores = maior prioridade)
        if ((current->prio_dynamic  < better->prio_dynamic) || 
            (current->prio_dynamic == better->prio_dynamic  &&
             current->id < better->id))
        {
            better = current;
        }
        current = current->next;
    }
    while (current != readyQueue); // Percorre toda a fila circular

    #ifdef DEBUG02
    printf("→ Escolhida: T%d\n", better->id);
    #endif

    // PASSO 2: Aplica envelhecimento (aging) - APENAS para tarefas de usuário
    // Tarefas de sistema (dispatcher, main) não sofrem aging
    if (better->user_task) {
        // Percorre TODAS as tarefas da fila novamente
        current = readyQueue;
        do {
            // Aplica aging apenas nas tarefas NÃO-ESCOLHIDAS
            if (current != better) {
                // Diminui o valor da prioridade (aumenta a prioridade real)
                if (current->prio_dynamic MENOS_PRIO PRIORITY_MAX) {
                    current->prio_dynamic += PRIORITY_ALPHA; // ALPHA = -1
                }
                // Garante que não ultrapasse o limite mínimo
                if (current->prio_dynamic MAIS_PRIO PRIORITY_MAX) {
                    current->prio_dynamic = PRIORITY_MAX;
                }
            }
            current = current->next;
        }
        while (current != readyQueue);

        // PASSO 3: Reset da prioridade dinâmica da tarefa escolhida
        // A tarefa escolhida volta à sua prioridade estática original
        better->prio_dynamic = better->prio_static;
    }
    
    // FIM DA ÁREA CRÍTICA
    PPOS_PREEMPT_ENABLE;
    return better;
}

/**
 * ============================================================================
 * SEÇÃO 3: FUNÇÃO DE TEMPO DO SISTEMA
 * ============================================================================
 */

/**
 * Retorna o tempo atual do sistema em milissegundos
 * 
 * Interface pública para obter o "relógio" do sistema.
 * É usada por todas as tarefas para saber quanto tempo se passou desde
 * a inicialização do sistema.
 * 
 * @return Tempo em milissegundos desde ppos_init()
 */
unsigned int systime() {
    return _systemTime;
}

/**
 * ============================================================================
 * SEÇÃO 4: FUNÇÕES DE GERENCIAMENTO DE PRIORIDADES
 * ============================================================================
 */

 /**
 * Ajusta a prioridade estática de uma tarefa
 * 
 * A prioridade estática é o valor "base" da tarefa, que não muda com o aging.
 * Quando uma tarefa é escolhida pelo scheduler, sua prioridade dinâmica
 * é resetada para este valor estático.
 * 
 * @param task Tarefa a ter a prioridade ajustada (NULL = tarefa atual)
 * @param prio Nova prioridade (-20 a +20, menor valor = maior prioridade)
 */
void task_setprio(task_t *task, int prio) {

    // Se task é NULL, assume que é a tarefa atualmente em execução
    if (task == NULL) {
        task = taskExec;
    }
    
    // Validação: garante que a prioridade está no intervalo válido
    // Prioridades fora deste intervalo são "cortadas" nos limites
    if (prio MAIS_PRIO PRIORITY_MAX)  prio = PRIORITY_MAX;
    if (prio MENOS_PRIO PRIORITY_MIN) prio = PRIORITY_MIN;
    
    // Atualiza tanto a prioridade estática quanto a dinâmica
    // A dinâmica será modificada pelo aging, a estática permanece
    task->prio_static  = prio;
    task->prio_dynamic = prio;
}

/**
 * Consulta a prioridade estática de uma tarefa
 * 
 * Retorna sempre a prioridade "original" da tarefa, não a que pode ter
 * sido modificada pelo aging (prioridade dinâmica).
 * 
 * @param task Tarefa a ser consultada (NULL = tarefa atual)
 * @return Prioridade estática da tarefa
 */
int task_getprio(task_t *task) {
    // Se task é NULL, assume que é a tarefa atualmente em execução
    if (task == NULL) {
        task = taskExec;
    }

    return task->prio_static;
}

/**
 * ============================================================================
 * SEÇÃO 5: SISTEMA DE PREEMPÇÃO POR TEMPO
 * ============================================================================
 * 
 * A preempção por tempo é o que permite que o sistema seja multitarefa.
 * Sem ela, uma tarefa poderia executar indefinidamente sem dar chance
 * para outras tarefas rodarem.
 * 
 * FUNCIONAMENTO:
 * 1. Timer UNIX dispara a cada 1ms (SIGALRM)
 * 2. Tratador do sinal incrementa relógio global e decrementa quantum
 * 3. Quando quantum zera, força task_yield() (troca de contexto)
 */

/**
 * Tratador de sinal para o temporizador (SIGALRM)
 * 
 * Esta função é chamada automaticamente pelo SO UNIX a cada 1ms.
 * É ela que implementa o "tick" do relógio do sistema e controla
 * os quantums das tarefas.
 * 
 * @param signum Número do sinal recebido (sempre SIGALRM neste caso)
 */
void interrupt_handler(int signum) {
    // TICK DO RELÓGIO GLOBAL
    // Incrementa o contador que representa o tempo desde a inicialização
    _systemTime++;

    #ifdef DEBUG02
        printf("\n[DEBUG02] Timer tick %d, task %d, quantum %d", systime(), taskExec->id, taskExec->quantum);
    #endif

    // CONTROLE DE QUANTUM
    // Só processa quantum para tarefas de usuário e se preempção está ativa
    
    // Verifica se é tarefa de usuário (não-sistema)
    if (!taskExec->user_task) {
        return;  // Tarefas de sistema não têm quantum limitado
    }

    // Decrementa o quantum da tarefa atual
    taskExec->quantum--;

    // PREEMPÇÃO POR QUANTUM
    // Se o quantum chegou a zero, força a tarefa a sair do processador
    if (taskExec->quantum <= 0 && PPOS_IS_PREEMPT_ACTIVE) {
        // task_yield() fará a tarefa voltar para fila de prontas
        // e o dispatcher escolherá a próxima tarefa    
        task_yield();
    }
}

/**
 * Inicializa o sistema de timer UNIX
 * 
 * Configura o mecanismo que simula o clock de hardware do computador.
 * Em um SO real, isso seria feito pelo hardware, mas como estamos
 * rodando como processo UNIX, usamos os timers do sistema.
 */
void timer_init() {
    // PASSO 1: Registra o tratador de sinal
    // Configura o que fazer quando receber o sinal SIGALRM
    timer_action.sa_handler = interrupt_handler;  // Função a ser chamada
    sigemptyset(&timer_action.sa_mask);           // Não bloqueia outros sinais
    timer_action.sa_flags   = 0;                  // Sem flags especiais
    
    // Instala o tratador no sistema operacional
    if (sigaction(SIGALRM, &timer_action, 0) < 0) {
        perror("Erro em sigaction: ");
        exit(1);
    }
    
    // PASSO 2: Configura os intervalos do timer
    timer.it_value.tv_usec    = TIMER_INTERVAL; // Primeiro disparo: 1000μs
    timer.it_value.tv_sec     = 0;              // 0 segundos
    timer.it_interval.tv_usec = TIMER_INTERVAL; // Disparos seguintes: 1000μs
    timer.it_interval.tv_sec  = 0;              // 0 segundos
    
    // PASSO 3: Ativa o timer
    // ITIMER_REAL: timer baseado em tempo real (wall clock)
    if (setitimer(ITIMER_REAL, &timer, 0) < 0) {
        perror("Erro em setitimer: ");
        exit(1);
    }
}

/**
 * ============================================================================
 * SEÇÃO 6: HOOKS DE INICIALIZAÇÃO DO SISTEMA
 * ============================================================================
 */

 /**
 * Hook chamado ANTES da inicialização do PingPongOS
 * 
 * Executado antes de qualquer estrutura do sistema ser criada.
 */
void before_ppos_init () {
    // put your customization here
#ifdef DEBUG
    printf("\ninit - BEFORE");
#endif    

}

/**
 * Hook chamado APÓS a inicialização do PingPongOS
 * 
 * - Inicializa o relógio global
 * - Configura o timer de preempção
 * - Define características da tarefa main
 * - Habilita preempção
 */
void after_ppos_init () {
    // put your customization here
#ifdef DEBUG
    printf("\ninit - AFTER");
#endif

    // INICIALIZAÇÃO DO RELÓGIO
    // O sistema "nasce" no tempo zero
    _systemTime = 0;

    // INICIALIZAÇÃO DO TIMER DE PREEMPÇÃO
    // A partir daqui, interrupt_handler() será chamado a cada 1ms
    timer_init();

    // CONFIGURAÇÃO DA TAREFA MAIN
    // Main é considerada tarefa de sistema (não sofre preempção por quantum)
    taskMain->user_task = 0;

    // HABILITA PREEMPÇÃO
    // Sistema começa com preempção ativa
    PPOS_PREEMPT_ENABLE;
}

/**
 * ============================================================================
 * SEÇÃO 7: HOOKS DE GERENCIAMENTO DE TAREFAS
 * ============================================================================
 */

 /**
 * Hook chamado ANTES da criação de uma tarefa
 * 
 * Desabilita preempção para que a criação de uma tarefa seja atômica.
 * Isso evita que o timer interrompa durante a inicialização de estruturas.
 * (Entretanto, apenas a Main cria tarefas e ela não pode ser preemptada).
 */
void before_task_create (task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_create - BEFORE - [%d]", task->id);
#endif

    // ÁREA CRÍTICA: Criação de tarefas não deve ser interrompida
    // Se o timer disparar no meio da criação, pode deixar estruturas
    // em estado inconsistente
    PPOS_PREEMPT_DISABLE;

}

/**
 * Hook chamado APÓS a criação de uma tarefa
 * 
 * Inicializa todos os campos específicos do sistema:
 * - Prioridades (estática e dinâmica)
 * - Quantum inicial
 * - Flags de tipo de tarefa
 * - Métricas de contabilização
 */
void after_task_create (task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_create - AFTER - [%d]", task->id);
#endif

    // INICIALIZAÇÃO DE PRIORIDADES
    // Toda tarefa nova nasce com prioridade padrão (0)
    task->prio_static  = PRIORITY_DEF;
    task->prio_dynamic = PRIORITY_DEF;

    // INICIALIZAÇÃO DE QUANTUM
    // Toda tarefa começa com quantum completo
    task->quantum      = QUANTUM_SIZE;

    // IDENTIFICAÇÃO DO TIPO DE TAREFA
    // Por convenção: ID > 1 = tarefa de usuário
    // ID 0 = main, ID 1 = dispatcher (tarefas de sistema)
    task->user_task    = (task->id > 1) ? 1 : 0;

    // INICIALIZAÇÃO DE MÉTRICAS DE CONTABILIZAÇÃO
    task->exec_start   = systime();     // Marca quando a tarefa foi criada
    task->proc_time    = 0;             // Tempo total usando processador
    task->last_proc    = 0;             // Quando começou a usar processador
    task->activations  = 0;             // Quantas vezes foi ativada
    task->running_time = 0;             // Tempo de execução em ticks
    
    // Reabilita preempção após a criação da tarefa
    PPOS_PREEMPT_ENABLE;

}

/**
 * Hook chamado ANTES da finalização de uma tarefa
 * 
 * Desabilita preempção para garantir que o processo de finalização
 * seja executado atomicamente.
 */
void before_task_exit () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_exit - BEFORE - [%d]", taskExec->id);
#endif

    // ÁREA CRÍTICA: Finalização não deve ser interrompida
    PPOS_PREEMPT_DISABLE;

}

/**
 * Hook chamado APÓS a finalização de uma tarefa
 * 
 * Aqui calculamos e exibimos as estatísticas finais da tarefa:
 * - Tempo total de execução (desde criação até morte)
 * - Tempo total de processador (quanto realmente usou CPU)
 * - Número de ativações (quantas vezes ganhou o processador)
 */
void after_task_exit () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_exit - AFTER- [%d]", taskExec->id);
#endif

    // CÁLCULO DE MÉTRICAS FINAIS
    // Tempo total = tempo atual - tempo de criação
    unsigned int task_total_time = systime() - taskExec->exec_start;

    // EXIBIÇÃO DE ESTATÍSTICAS
    // Formato padrão requerido pelo projeto
    printf("Task %d exit: execution time %u ms, processor time %u ms, %u activations\n",
        taskExec->id,           // id da tarefa
        task_total_time,        // Tempo total "de vida"
        taskExec->proc_time,    // Tempo efetivo usando CPU
        taskExec->activations); // Número de ativações

    // Nota: Preempção será reabilitada em after_task_switch

}

/**
 * Hook chamado ANTES de uma troca de contexto
 * 
 * Atualiza as métricas da tarefa que está SAINDO do processador.
 * É crucial fazer isso aqui porque depois da troca não teremos
 * mais acesso fácil à tarefa anterior.
 */
void before_task_switch ( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_switch - BEFORE - [%d -> %d]", taskExec->id, task->id);
#endif

    // ÁREA CRÍTICA: Troca de contexto é operação sensível
    PPOS_PREEMPT_DISABLE;

    // CONTABILIZAÇÃO DA TAREFA QUE ESTÁ SAINDO
    // Só contabiliza para tarefas de usuário (user_task) e
    // Se a tarefa estava realmente usando o processador
    if (taskExec && taskExec->user_task && taskExec->last_proc > 0) {
        // Calcula quanto tempo usou nesta "rodada"
        unsigned int time_slice = systime() - taskExec->last_proc;
        // Acumula no tempo total de processador
        taskExec->proc_time += time_slice;
    }

}

/**
 * Hook chamado APÓS uma troca de contexto
 * 
 * Atualiza as métricas da tarefa que está ENTRANDO no processador.
 * Também redefine o quantum para a nova tarefa.
 */
void after_task_switch ( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_switch - AFTER - [%d -> %d]", taskExec->id, task->id);
#endif

    // CONTABILIZAÇÃO DA TAREFA QUE ESTÁ ENTRANDO
    if (task && task->user_task) {

        // Incrementa contador de ativações
        task->activations++;
        
        // Marca o momento em que ganhou o processador
        task->last_proc = systime();
        
        // REINICIALIZAÇÃO DO QUANTUM
        // Toda vez que uma tarefa ganha o processador, recebe quantum completo
        task->quantum   = QUANTUM_SIZE;
    }

    // Reabilita preempção após a troca estar completa
    PPOS_PREEMPT_ENABLE;

}

/**
 * Hook chamado ANTES de um yield voluntário
 * 
 * Desabilita preempção para que o processo de yield seja atômico.
 */
void before_task_yield () {
    // put your customization here
#ifdef DEBUG02
    printf("\ntask_yield - BEFORE - [%d]", taskExec->id);
#endif

    // ÁREA CRÍTICA: Yield não deve ser interrompido
    PPOS_PREEMPT_DISABLE;

}

/**
 * Hook chamado APÓS um yield voluntário
 * 
 * A preempção será reabilitada em after_task_switch quando
 * a nova tarefa assumir o processador.
 */
void after_task_yield () {
    // put your customization here
#ifdef DEBUG02
    printf("\ntask_yield - AFTER - [%d]", taskExec->id);
#endif

    // Nota: Preempção será reabilitada em after_task_switch

}

/**
 * Hook chamado ANTES de suspender uma tarefa
 * 
 * Usado quando uma tarefa precisa esperar por algum evento
 * (como I/O, semáforo, etc.)
 */
void before_task_suspend( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - BEFORE - [%d]", task->id);
#endif

    // ÁREA CRÍTICA: Suspensão não deve ser interrompida
    PPOS_PREEMPT_DISABLE;

}

/**
 * Hook chamado APÓS suspender uma tarefa
 */
void after_task_suspend( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - AFTER - [%d]", task->id);
#endif

    // Reabilita preempção
    PPOS_PREEMPT_ENABLE;

}

/**
 * Hook chamado ANTES de retomar uma tarefa suspensa
 */
void before_task_resume(task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - BEFORE - [%d]", task->id);
#endif

    // ÁREA CRÍTICA: Retomada não deve ser interrompida
    PPOS_PREEMPT_DISABLE;

}

/**
 * Hook chamado APÓS retomar uma tarefa suspensa
 */
void after_task_resume(task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - AFTER - [%d]", task->id);
#endif

    // Reabilita preempção
    PPOS_PREEMPT_ENABLE;

}

/**
 * Hook chamado ANTES de uma tarefa dormir
 * 
 * task_sleep() suspende a tarefa por um tempo determinado
 */
void before_task_sleep () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - BEFORE - [%d]", taskExec->id);
#endif

    // ÁREA CRÍTICA: Colocar para dormir não deve ser interrompido
    PPOS_PREEMPT_DISABLE;   

}

/**
 * Hook chamado APÓS uma tarefa dormir
 * 
 * A preempção será reabilitada quando a tarefa acordar
 */
void after_task_sleep () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - AFTER - [%d]", taskExec->id);
#endif

    // Nota: Preempção será reabilitada em after_task_switch

}

/**
 * Hook chamado ANTES de um join
 * 
 * task_join() faz uma tarefa esperar outra terminar
 */
int before_task_join (task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - BEFORE - [%d]", taskExec->id);
#endif

    // ÁREA CRÍTICA: Join não deve ser interrompido
    PPOS_PREEMPT_DISABLE;

    return 0;
    
}

/**
 * Hook chamado APÓS um join
 */
int after_task_join (task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - AFTER - [%d]", taskExec->id);
#endif

    // Reabilita preempção
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