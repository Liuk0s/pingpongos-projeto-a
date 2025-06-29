/**
 * ============================================================================
 * PingPongOS - Projeto B
 * Implementação do Gerenciador de Discos e Escalonador de Requisições
 * 
 * Alunos: Lucas Giovanni Thuler
 * 
 * Implementa um gerenciador de disco virtual com suporte a diferentes
 * políticas de escalonamento (FCFS, SSTF, CSCAN) e controle de acesso
 * concorrente através de semáforos.
 * ============================================================================
 */

#define _XOPEN_SOURCE 600     // Macro de compatibilidade para sigaction e sigemptyset
#include <signal.h>            // sigaction, SIGUSR1
#include "ppos.h"              // Definições principais
#include "ppos-core-globals.h" // se você usar variáveis globais
#include "disk-driver.h"       // disk_cmd e constantes
#include "ppos_disk.h"
#include <limits.h>


/**
 * ============================================================================
 * CONFIGURAÇÕES E ESTRUTURAS AUXILIARES
 * ============================================================================
 */

#define SCHEDULER_FCFS   1  // First Come, First Served
#define SCHEDULER_SSTF   2  // Shortest Seek Time First  
#define SCHEDULER_CSCAN  3  // Circular Scan
#define ERROR_INVALID   -1  // Código de erro padrão

// Estrutura para rastreamento de performance do disco
typedef struct {
    int current_head_position;  // Posição atual da cabeça de leitura/escrita
    int total_head_movements;   // Total de blocos percorridos pela cabeça
    int requests_processed;     // Número de requisições processadas
    int active_policy;          // Política de escalonamento ativa
    int last_direction;         // Última direção do movimento (CSCAN)
} disk_performance_tracker_t;

// Estrutura para estatísticas operacionais
typedef struct {
    unsigned int read_operations;       // Contador de operações de leitura
    unsigned int write_operations;      // Contador de operações de escrita
    unsigned int total_seek_distance;   // Distância total percorrida
    unsigned int average_response_time; // Tempo médio de resposta
} operation_stats_t;


/**
 * ============================================================================
 * VARIÁVEIS GLOBAIS E ESTADO DO SISTEMA
 * ============================================================================
 */

disk_t disk;                                        // Estrutura principal do disco
task_t taskDiskMgr;                                 // Tarefa gerenciadora do disco
static disk_performance_tracker_t perf_tracker;     // Métricas de performance e posicionamento
static operation_stats_t stats;                     // Estatísticas operacionais detalhadas
static volatile int system_shutdown_requested = 0;  // Flag para encerramento controlado

// Protótipos das funções principais
void bodyDiskManager(void *arg);
void diskSignalHandler(int signum);
diskrequest_t* disk_scheduler(void);
diskrequest_t* createDiskRequest(int operation, int block, void* buffer);

// Protótipos das funções auxiliares
static void updatePerformanceMetrics(int old_pos, int new_pos);
static void printSystemStatistics(void);
static void processCompletionEvents(void);
static void processNewRequests(void);
static int executeRequest(diskrequest_t* request);

static diskrequest_t* fcfs_scheduler(void);
static diskrequest_t* sstf_scheduler(void);
static diskrequest_t* cscan_scheduler(void);


/**
 * ============================================================================
 * INICIALIZAÇÃO DO SISTEMA
 * ============================================================================
 */

int disk_mgr_init(int *numBlocks, int *blockSize) {
    
    // Validação de parâmetros de entrada
    if (!numBlocks || !blockSize) {
        return ERROR_INVALID;
    }

    // Inicialização do hardware virtual do disco
    if(disk_cmd(DISK_CMD_INIT, 0, 0) < 0) {
        return ERROR_INVALID;
    }
    
    // Consulta características físicas do disco
    int disk_size  = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    int block_size = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);

    if (disk_size < 0 || block_size < 0) {
        return ERROR_INVALID;
    }

    // Configuração da estrutura principal do disco
    disk.numBlocks    = disk_size;      // Número total de blocos
    disk.blockSize    = block_size;     // Tamanho de cada bloco
    disk.diskQueue    = NULL;           // Fila de tarefas suspensas
    disk.requestQueue = NULL;           // Fila de requisições pendentes
    disk.livre        = 1;              // Disco inicialmente livre
    disk.sinal        = 0;              // Sem sinais pendentes

    // Criação dos semáforos de controle de concorrência
    sem_create(&disk.semaforo, 1);          // Semáforo principal do disco
    sem_create(&disk.semaforo_queue, 1);    // Semáforo da fila de requisições

    // Configuração inicial do tracker de performance
    #ifdef SCHEDULER_DEFAULT
        perf_tracker.active_policy = SCHEDULER_DEFAULT;  // Política definida na compilação
    #else
        perf_tracker.active_policy = SCHEDULER_CSCAN;    // Política padrão se não definida
    #endif
    perf_tracker.current_head_position = 0;                // Cabeça inicia em 0
    perf_tracker.total_head_movements  = 0;                // Sem movimentos ainda
    perf_tracker.requests_processed    = 0;                // Nenhuma requisição processada
    perf_tracker.last_direction        = 1;                // Direção crescente (CSCAN)

    // Inicialização das estatísticas operacionais
    stats.read_operations       = 0;    // Contador de leituras
    stats.write_operations      = 0;    // Contador de escritas
    stats.total_seek_distance   = 0;    // Distância total percorrida
    stats.average_response_time = 0;    // Tempo médio de resposta

    // Criação da tarefa gerenciadora do disco
    task_create(&taskDiskMgr, bodyDiskManager, NULL);

    // Configuração do handler de sinais SIGUSR1
    struct sigaction disk_handler;
    disk_handler.sa_handler = diskSignalHandler;
    sigemptyset(&disk_handler.sa_mask);
    disk_handler.sa_flags = 0;

    if (sigaction(SIGUSR1, &disk_handler, NULL) < 0) {
        perror("Erro: falha na criação do handler");
        exit(1);
    }

    // Retorna informações do disco para o chamador
    *numBlocks = disk_size;
    *blockSize = block_size;

    return 0;
}


/**
 * ============================================================================
 * INTERFACE DE ACESSO AO DISCO
 * ============================================================================
 */

/**
 * Lê um bloco do disco para um buffer
 * 
 * Cria uma requisição de leitura, adiciona à fila de processamento e
 * suspende a tarefa atual até que a operação seja concluída.
 * 
 * @param block Número do bloco a ser lido (0 a numBlocks-1)
 * @param buffer Buffer onde os dados lidos serão armazenados
 * @return 0 em sucesso, ERROR_INVALID em caso de erro
 */
int disk_block_read(int block, void *buffer) {

    // Validação de parâmetros
    if(!buffer) {
        return ERROR_INVALID;
    }
    if (block < 0 || block >= disk.numBlocks) {
        return ERROR_INVALID;
    }

    // Criação da requisição de leitura
    diskrequest_t* request = createDiskRequest(DISK_CMD_READ, block, buffer);
    if(!request) {
        return ERROR_INVALID;
    }

    // Inserção na fila de requisições
    sem_down(&disk.semaforo_queue);
    queue_append((queue_t**)&disk.requestQueue, (queue_t*)request);
    sem_up(&disk.semaforo_queue);

    // Suspende tarefa atual até conclusão da operação
    task_suspend(taskExec, &disk.diskQueue);
    task_yield();

    // Atualiza estatísticas de operações de leitura
    stats.read_operations++;
    
    return 0;
}

/**
 * Escreve um bloco do buffer para o disco
 * 
 * Cria uma requisição de escrita, adiciona à fila de processamento e
 * suspende a tarefa atual até que a operação seja concluída.
 * 
 * @param block Número do bloco a ser escrito (0 a numBlocks-1)
 * @param buffer Buffer contendo os dados a serem escritos
 * @return 0 em sucesso, ERROR_INVALID em caso de erro
 */
int disk_block_write(int block, void *buffer) {

    // Validação de parâmetros
    if (!buffer) {
        return ERROR_INVALID;
    }
    if (block < 0 || block >= disk.numBlocks) {
        return ERROR_INVALID;
    }

    // Criação da requisição de escrita
    diskrequest_t* request = createDiskRequest(DISK_CMD_WRITE, block, buffer);
    if(!request) {
        return ERROR_INVALID;
    }

    // Inserção na fila de requisições
    sem_down(&disk.semaforo_queue);
    queue_append((queue_t**)&disk.requestQueue, (queue_t*)request);
    sem_up(&disk.semaforo_queue);

    // Suspende tarefa atual até conclusão da operação
    task_suspend(taskExec, &disk.diskQueue);
    task_yield();

    // Atualiza estatísticas de operações de escrita
    stats.write_operations++;

    return 0;
}


/**
 * ============================================================================
 * TAREFA GERENCIADORA
 * ============================================================================
 */

/**
 * Corpo principal da tarefa gerenciadora do disco
 * 
 * Loop infinito que processa eventos de conclusão e novas requisições,
 * controlando todo o fluxo de operações do disco virtual.
 * 
 * @param arg Argumento não utilizado (NULL)
 */
void bodyDiskManager(void *arg) {
    while(1) {

        // Verifica se deve encerrar o sistema
        if (system_shutdown_requested && !disk.requestQueue && !disk.diskQueue) {
            printSystemStatistics();
            task_exit(0);
        }
        
        // Processa eventos de conclusão de operações
        processCompletionEvents();
        
        // Processa novas requisições pendentes
        processNewRequests();
        
        // Cede processador para outras tarefas
        task_yield();
    }
}

/**
 * Processa eventos de conclusão de operações do disco
 * 
 * Verifica se há sinais pendentes do hardware e libera tarefas
 * que estavam aguardando conclusão de operações.
 */
static void processCompletionEvents(void) {
    sem_down(&disk.semaforo);
    
    if (disk.sinal) {
        disk.sinal = 0;         // Limpa flag de sinal
        disk.livre = 1;         // Marca disco como disponível
        
        // Acorda próxima tarefa da fila de espera
        if (disk.diskQueue) {
            task_resume(disk.diskQueue);
        }
    }
    
    sem_up(&disk.semaforo);
}

/**
 * Processa novas requisições de acesso ao disco
 * 
 * Verifica se há requisições pendentes e o disco está livre,
 * selecionando e executando a próxima requisição.
 */
static void processNewRequests(void) {
    sem_down(&disk.semaforo);
    
    // Verifica se disco está livre e há requisições pendentes
    if (disk.livre && disk.requestQueue) {
        // Seleciona próxima requisição usando algoritmo ativo
        diskrequest_t* next_request = disk_scheduler();
        
        if (next_request) {
            // Remove requisição da fila
            sem_down(&disk.semaforo_queue);
            queue_remove((queue_t**)&disk.requestQueue, (queue_t*)next_request);
            sem_up(&disk.semaforo_queue);
            
            // Executa a requisição selecionada
            executeRequest(next_request);
            free(next_request);
        }
    }
    
    sem_up(&disk.semaforo);
}


/**
 * ============================================================================
 * ALGORITMOS DE ESCALONAMENTO
 * ============================================================================
 */

/**
 * Dispatcher principal dos algoritmos de escalonamento
 * 
 * Chama o algoritmo de escalonamento apropriado baseado na
 * política atualmente ativa no sistema.
 * 
 * @return Ponteiro para próxima requisição a ser processada
 */
diskrequest_t* disk_scheduler(void) {
    switch (perf_tracker.active_policy) {
        case SCHEDULER_FCFS:
            return fcfs_scheduler();
        case SCHEDULER_SSTF:
            return sstf_scheduler();
        case SCHEDULER_CSCAN:
            return cscan_scheduler();
        default:
            return fcfs_scheduler();
    }
}

/**
 * Algoritmo FCFS - First Come, First Served
 * 
 * Implementa escalonamento simples: primeira requisição que
 * chegou é a primeira a ser atendida.
 * 
 * @return Ponteiro para primeira requisição da fila
 */
static diskrequest_t* fcfs_scheduler(void) {
    return (diskrequest_t*)disk.requestQueue;
}

/**
 * Algoritmo SSTF - Shortest Seek Time First
 * 
 * Seleciona a requisição que requer menor movimento da cabeça
 * do disco a partir da posição atual.
 * 
 * @return Ponteiro para requisição com menor distância de seek
 */
static diskrequest_t* sstf_scheduler(void) {
    if (!disk.requestQueue) return NULL;
    
    diskrequest_t* best_request = NULL;         // Melhor requisição encontrada
    diskrequest_t* current = (diskrequest_t*)disk.requestQueue;
    int minimal_distance = INT_MAX;             // Menor distância encontrada
    
    // Busca requisição mais próxima da posição atual da cabeça
    do {
        int seek_distance = abs(current->block - perf_tracker.current_head_position);
        if (seek_distance < minimal_distance) {
            minimal_distance = seek_distance;
            best_request = current;
        }
        current = current->next;
    } while (current != (diskrequest_t*)disk.requestQueue);
    
    return best_request;
}

/**
 * Algoritmo CSCAN - Circular Scan
 * 
 * Move a cabeça sempre em direção crescente. Quando chega ao fim,
 * retorna ao início do disco (movimento circular).
 * 
 * @return Ponteiro para próxima requisição na direção crescente
 */
static diskrequest_t* cscan_scheduler(void) {
    if (!disk.requestQueue) return NULL;
    
    diskrequest_t* forward_candidate = NULL;    // Candidato na direção crescente
    diskrequest_t* wrap_candidate = NULL;       // Candidato para wrap circular
    diskrequest_t* current = (diskrequest_t*)disk.requestQueue;
    int min_forward_distance = INT_MAX;         // Menor distância à frente
    int min_wrap_distance = INT_MAX;            // Menor posição para wrap
    
    // Busca candidatos em direção crescente e para wrap circular
    do {
        int block_position = current->block;
        
        if (block_position >= perf_tracker.current_head_position) {
            // Candidato na direção crescente (preferencial)
            int distance = block_position - perf_tracker.current_head_position;
            if (distance < min_forward_distance) {
                min_forward_distance = distance;
                forward_candidate = current;
            }
        } else {
            // Candidato para wrap circular (volta ao início)
            if (block_position < min_wrap_distance) {
                min_wrap_distance = block_position;
                wrap_candidate = current;
            }
        }
        
        current = current->next;
    } while (current != (diskrequest_t*)disk.requestQueue);
    
    // Prioriza direção crescente, senão faz wrap circular
    return forward_candidate ? forward_candidate : wrap_candidate;
}


/**
 * ============================================================================
 * FUNÇÕES AUXILIARES
 * ============================================================================
 */

/**
 * Cria uma nova requisição de disco
 * 
 * Aloca e inicializa uma estrutura de requisição com os
 * parâmetros fornecidos.
 * 
 * @param operation Tipo de operação (DISK_CMD_READ ou DISK_CMD_WRITE)
 * @param block Número do bloco alvo
 * @param buffer Buffer de dados
 * @return Ponteiro para requisição criada ou NULL em erro
 */
diskrequest_t* createDiskRequest(int operation, int block, void *buffer) {

    diskrequest_t* request = malloc(sizeof(diskrequest_t));
    if (!request) {
        return NULL;
    }

    request->prev = NULL;               // Inicializa ponteiros da lista
    request->next = NULL;               // Inicializa ponteiros da lista
    request->task = taskExec;           // Tarefa solicitante atual
    request->operation = operation;     // Tipo de operação
    request->block = block;             // Bloco alvo no disco
    request->buffer = buffer;           // Buffer de dados

    return request;
}

/**
 * Executa uma requisição de disco
 * 
 * Atualiza métricas, determina comando apropriado e executa
 * a operação no hardware do disco.
 * 
 * @param request Ponteiro para requisição a ser executada
 * @return 0 em sucesso
 */
static int executeRequest(diskrequest_t* request) {
    
    // Atualiza métricas de movimentação da cabeça
    updatePerformanceMetrics(perf_tracker.current_head_position, request->block);
    
    // Determina comando apropriado para o hardware
    int disk_command;
    if (request->operation == 1) {
        disk_command = DISK_CMD_READ;
    } 
    else {
        disk_command = DISK_CMD_WRITE;
    }
    
    // Executa operação no hardware do disco
    disk_cmd(disk_command, request->block, request->buffer);
    
    disk.livre = 0;                     // Marca disco como ocupado
    perf_tracker.requests_processed++;  // Incrementa contador de requisições
    
    return 0;
}


/**
 * Atualiza métricas de performance do sistema
 * 
 * Calcula movimento da cabeça e atualiza estatísticas
 * de desempenho do disco.
 * 
 * @param old_pos Posição anterior da cabeça
 * @param new_pos Nova posição da cabeça
 */
static void updatePerformanceMetrics(int old_pos, int new_pos) {
    int movement = abs(new_pos - old_pos);          // Calcula movimento
    perf_tracker.total_head_movements += movement;  // Acumula movimento total
    perf_tracker.current_head_position = new_pos;   // Atualiza posição atual
    stats.total_seek_distance += movement;          // Atualiza estatística de seek
}

/**
 * Imprime estatísticas do sistema
 * 
 * Exibe relatório de performance incluindo política ativa,
 * operações realizadas e métricas de eficiência.
 */
static void printSystemStatistics(void) {
    printf("\n=== RELATÓRIO DE PERFORMANCE DO SISTEMA ===\n");
    
    // Determina nome da política ativa
    char* policy_name;
    if (perf_tracker.active_policy == SCHEDULER_FCFS) {
        policy_name = "FCFS";
    } else if (perf_tracker.active_policy == SCHEDULER_SSTF) {
        policy_name = "SSTF";
    } else {
        policy_name = "CSCAN";
    }
    
    printf(" -- Política ativa: %s\n", policy_name);
    printf(" -- Requisições processadas: %d\n", perf_tracker.requests_processed);
    printf(" -- Operações de leitura: %u\n", stats.read_operations);
    printf(" -- Operações de escrita: %u\n", stats.write_operations);
    printf(" -- Movimentação total da cabeça: %d blocos\n", perf_tracker.total_head_movements);
    if (perf_tracker.requests_processed > 0) {
        printf(" -- Movimentação média por requisição: %.2f blocos\n",
               (float)perf_tracker.total_head_movements / perf_tracker.requests_processed);
    }
    printf(" -- Tempo total de execução: %u ms\n", systime());
    printf("===========================================\n");
}


/**
 * ============================================================================
 * HANDLERS DE SINAL E SHUTDOWN
 * ============================================================================
 */

/**
 * Handler para sinais SIGUSR1 do disco
 * 
 * Função chamada automaticamente quando o disco virtual
 * completa uma operação. Sinaliza conclusão para o gerenciador.
 * 
 * @param signum Número do sinal recebido (SIGUSR1)
 */
void diskSignalHandler(int signum) {
    disk.sinal = 1;  // Sinaliza conclusão de operação
}

/**
 * Solicita encerramento gracioso do sistema
 * 
 * Define flag para que o gerenciador termine após processar
 * todas as requisições pendentes.
 */
void disk_mgr_shutdown(void) {
    system_shutdown_requested = 1;
}