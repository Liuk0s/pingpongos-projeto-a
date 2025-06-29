# PingPongOS - Projetos A e B: Sistema Operacional Educacional

Este repositório contém a implementação dos **Projetos A e B** do PingPongOS, um sistema operacional educacional desenvolvido para a disciplina de Sistemas Operacionais.

## Autores da Implementação
- **Iaritzza Bielinki** (Projeto A)
- **Lucas Giovanni Thuler** (Projetos A e B)

## Créditos e Contexto Acadêmico
- **Disciplina**: Sistemas Operacionais (EC) - UTFPR
- **Professor**: Marco Aurélio Wehrmeister
- **Sistema Base**: PingPongOS - criado pelo Prof. Carlos Maziero (DInf/UFPR)
- **Documentação oficial**: https://wiki.inf.ufpr.br/maziero/doku.php?id=so:pingpongos

*A autoria de todo o material base utilizado é do prof. Carlos Maziero (DInf/UFPR), que gentilmente o cedeu para aplicação nesta disciplina.*

## Sobre os Projetos

O PingPongOS é um sistema operacional educacional criado para demonstrar conceitos fundamentais de sistemas operacionais. Este repositório implementa as funcionalidades de dois projetos distintos:

### **Projeto A - Escalonador, Preempção e Contabilização**
1. **Escalonador por Prioridades com Aging**
2. **Sistema de Preempção por Tempo**
3. **Contabilização de Uso do Processador**

### **Projeto B - Gerenciador de Disco**
1. **Gerenciador de Disco Virtual**
2. **Escalonadores de Requisições de Disco (FCFS, SSTF, CSCAN)**
3. **Sistema de Métricas de Performance**

---

# Projeto A - Escalonador, Preempção e Contabilização

## Funcionalidades Implementadas

### 1. Escalonador por Prioridades (`scheduler()`)
- **Algoritmo**: Escolhe tarefa com menor valor de prioridade dinâmica
- **Aging**: Tarefas não-escolhidas ganham prioridade (-1 a cada ciclo)
- **Desempate**: Critério por ID das tarefas (FIFO)
- **Escopo**: Apenas tarefas de usuário (ID > 1) participam do aging
- **Reset**: Tarefa escolhida volta à prioridade estática original

### 2. Sistema de Preempção por Tempo
- **Timer UNIX**: Usa `setitimer()` e `SIGALRM` para simular clock de hardware
- **Quantum**: 20 ticks por tarefa (configurável via `QUANTUM_SIZE`)
- **Intervalo**: Timer dispara a cada 1ms (`TIMER_INTERVAL`)
- **Controle**: Apenas tarefas de usuário sofrem preempção por quantum
- **Handler**: `interrupt_handler()` gerencia tick do relógio e controle de quantum

### 3. Contabilização de Recursos
- **Métricas por tarefa**:
  - Tempo total de execução (criação até término)
  - Tempo efetivo de processador (uso real de CPU)
  - Número de ativações (quantas vezes ganhou o processador)
- **Hooks implementados**: Before/after para create, exit, switch, yield, suspend, resume, sleep, join
- **Relatório final**: Estatísticas exibidas quando tarefa termina

### 4. Funções de Suporte
- **`systime()`**: Retorna tempo do sistema em milissegundos
- **`task_setprio()`**: Define prioridade estática de uma tarefa
- **`task_getprio()`**: Consulta prioridade estática de uma tarefa
- **Hooks de sistema**: Instrumentação do ciclo de vida das tarefas

---

# Projeto B - Gerenciador de Disco

## Funcionalidades Implementadas

### 1. Gerenciador de Disco Virtual
- **Interface de acesso**: `disk_mgr_init()`, `disk_block_read()`, `disk_block_write()`
- **Tarefa gerenciadora**: Processa requisições de forma assíncrona
- **Controle de concorrência**: Semáforos para proteção de estruturas críticas
- **Handler de sinais**: Processa sinais SIGUSR1 do disco virtual

### 2. Algoritmos de Escalonamento de Disco

#### **FCFS (First Come, First Served)**
- **Estratégia**: Primeira requisição que chega é a primeira atendida
- **Características**: Simples, justo, mas pode não ser eficiente
- **Uso**: Adequado para cargas leves ou sistemas simples

#### **SSTF (Shortest Seek Time First)**
- **Estratégia**: Atende requisição mais próxima da posição atual da cabeça
- **Características**: Minimiza movimento da cabeça, otimiza throughput
- **Limitação**: Pode causar starvation para requisições distantes

#### **CSCAN (Circular Scan)**
- **Estratégia**: Move cabeça sempre em direção crescente, retorna ao início circularmente
- **Características**: Garante tempo limitado de espera, evita starvation
- **Vantagem**: Balanceamento entre eficiência e fairness

### 3. Sistema de Métricas
- **Rastreamento de performance**: Posição da cabeça, movimentos totais, requisições processadas
- **Estatísticas operacionais**: Contadores de leitura/escrita, distância percorrida, tempo de resposta
- **Relatórios comparativos**: Análise de performance entre algoritmos
- **Exportação de dados**: Métricas exportáveis em formato CSV

### 4. Arquitetura Modular
- **Processamento separado**: Eventos de conclusão e novas requisições em módulos distintos
- **Configuração flexível**: Política de escalonamento configurável via compilação
- **Validação de parâmetros**: Verificação de parâmetros e estados do sistema

## Estrutura do Repositório

```
projeto/
├── entrega/                    # Arquivos para submissão acadêmica
│   ├── pa-ppos-core-aux.c      # Projeto A
│   ├── pa-ppos-core-aux-modificacoes.txt
│   ├── pa-ppos_data.h
│   ├── pb-ppos_disk.c          # Projeto B
│   ├── pb-ppos_disk.h
│   ├── pb-ppos-core-task-aux.c
│   └── pb-ppos_data.h
├── educational/                # Versões educacionais e fortemente comentadas (apenas do projeto A, por enquanto)
│   ├── ppos-core-aux-tutorial.c
│   └── ppos_disk-tutorial.c
├── expected-output/            # Saídas de referência dos testes
│   ├── pingpong-dispatcher.txt
│   ├── pingpong-scheduler.txt
│   ├── pingpong-preempcao.txt
│   ├── pingpong-contab-prio.txt
│   ├── pingpong-preempcao-stress.txt
│   ├── pingpong-disco1.txt
│   └── pingpong-disco2.txt
├── ppos-core-aux.c           # Implementação Projeto A
├── ppos_disk.c               # Implementação Projeto B
├── ppos_disk.h               # Interface Projeto B
├── queue.o                   # Biblioteca de filas (fornecida)
├── ppos-all.o               # Núcleo do PingPongOS (fornecido)
├── disk-driver.o            # Driver de disco virtual (fornecido)
├── pingpong-*.c             # Programas de teste (fornecidos)
├── disk.dat                 # Dados do disco virtual
├── bin/                     # Executáveis compilados (gerado)
├── output/                  # Resultados dos testes (gerado)
├── README.md
└── Makefile
```

## Makefile

O sistema inclui um makefile que detecta automaticamente qual projeto está disponível baseado nos arquivos presentes no diretório.

### Detecção Automática
```bash
make all                    # Detecta e compila projeto disponível
make detect-project         # Mostra qual projeto foi detectado
make show-project-status    # Status dos projetos
```

### Compilação por Projeto
```bash
# Força projeto específico
make project-a              # Compila Projeto A
make project-b              # Compila Projeto B
make PROJECT=A all          # Via variável de ambiente
make PROJECT=B all          # Via variável de ambiente
```

### Projeto A - Escalonador
```bash
# Compilação
make project-a

# Testes
make test-project-a
make run-scheduler          # Execução na tela
make run-preempcao
make run-contab
```

### Projeto B - Disco (⚠️ Modifica disk.dat)

#### Compilação (não executa)
```bash
make project-b              # Compila todos os schedulers
make disco1-all             # Compila disco1 (todos schedulers)
make disco2-all             # Compila disco2 (todos schedulers)
make scheduler-fcfs         # Compila apenas FCFS
make scheduler-sstf         # Compila apenas SSTF
make scheduler-cscan        # Compila apenas CSCAN
```

#### Gerenciamento de Backup
```bash
make backup-disk            # Faça backup primeiro!
make restore-disk           # Restaura disk.dat do backup
```

#### Testes Individuais
```bash
# Por teste específico
make disco1-fcfs            # Disco1 com FCFS
make disco1-sstf            # Disco1 com SSTF
make disco1-cscan           # Disco1 com CSCAN
make disco2-fcfs            # Disco2 com FCFS
make disco2-sstf            # Disco2 com SSTF
make disco2-cscan           # Disco2 com CSCAN

# Agrupados
make disco1-all-tests       # Todos os testes do disco1
make disco2-all-tests       # Todos os testes do disco2
make test-fcfs-only         # Apenas FCFS (disco1+disco2)
make test-sstf-only         # Apenas SSTF (disco1+disco2)
make test-cscan-only        # Apenas CSCAN (disco1+disco2)
```

#### Execução na Tela
```bash
make run-disco1-fcfs        # Execução direta na tela
make run-disco2-sstf        # (útil para debug)
make run-disco2-cscan
```

#### Análise Comparativa
```bash
make test-project-b         # Executa todos e gera relatório
make compare-disk-results   # Gera relatório comparativo
make extract-disk-metrics   # Extrai métricas para CSV
```

### Fluxo Recomendado (Projeto B)
```bash
# 1. Backup de segurança
make backup-disk

# 2. Compilação
make project-b

# 3. Teste individual
make disco1-fcfs

# 4. Análise completa (opcional)
make test-project-b

# 5. Restauração (se necessário)
make restore-disk
```

## Pré-requisitos
- GCC (GNU Compiler Collection)
- Make
- Sistema Unix/Linux
- Arquivos base do PingPongOS:
  - `queue.o`, `ppos-all.o` (objetos fornecidos)
  - `disk-driver.o` (para Projeto B)
  - `pingpong-*.c` (programas de teste)
  - `disk.dat` (dados do disco virtual para Projeto B)

## Configuração Inicial
```bash
# Clone o repositório
git clone [seu-repositório]
cd projeto

# Verifique arquivos necessários
make check-files

# Compile projeto desejado
make all                    # Detecção automática
# ou
make project-a              # Força Projeto A
make project-b              # Força Projeto B
```

## Testes Disponíveis

### Projeto A
- **`pingpong-dispatcher`**: Testa funcionamento básico do dispatcher
- **`pingpong-scheduler`**: Valida escalonador por prioridades e aging
- **`pingpong-preempcao`**: Testa sistema de preempção por tempo
- **`pingpong-contab-prio`**: Valida contabilização e ajuste de prioridades
- **`pingpong-preempcao-stress`**: Teste de stress da preempção

### Projeto B
- **`pingpong-disco1`**: Teste básico sequencial do disco
- **`pingpong-disco2`**: Teste com múltiplas tarefas simultâneas

Cada teste do Projeto B é compilado com os 3 algoritmos de escalonamento (FCFS, SSTF, CSCAN) para análise comparativa.

## Análise de Performance (Projeto B)

### Métricas Coletadas
- **Requisições processadas**: Número total de operações
- **Movimentação da cabeça**: Blocos percorridos total e médio
- **Tempo de execução**: Duração total do teste
- **Operações por tipo**: Contadores de leitura e escrita

### Relatórios Gerados
- **Comparativo textual**: `output/resumo-comparativo.txt`
- **Dados estruturados**: `output/metricas.csv`
- **Logs detalhados**: `output/disco{1,2}-{fcfs,sstf,cscan}.txt`

### Exemplo de Saída
```
=== RELATÓRIO DE PERFORMANCE DO SISTEMA ===
 -- Política ativa: CSCAN
 -- Requisições processadas: 512
 -- Operações de leitura: 256
 -- Operações de escrita: 256
 -- Movimentação total da cabeça: 6848 blocos
 -- Movimentação média por requisição: 13.38 blocos
 -- Tempo total de execução: 29146 ms
===========================================
```

## Decisões de Implementação

### Projeto A
#### Controle de Preempção
- Operações críticas são protegidas com `PPOS_PREEMPT_DISABLE/ENABLE`
- Garante atomicidade durante modificação de estruturas do sistema
- Evita condições de corrida no escalonador e gerenciamento de tarefas

#### Classificação de Tarefas
- **Tarefas de sistema** (ID ≤ 1): main, dispatcher
- **Tarefas de usuário** (ID > 1): participam do aging e sofrem preempção

#### Aging e Prioridades
- Fator alpha = -1 (melhora prioridade das não-escolhidas)
- Limites: -20 (maior prioridade) a +20 (menor prioridade)
- Reset automático da tarefa escolhida para prioridade estática

### Projeto B
#### Arquitetura Modular
- Separação entre processamento de eventos e novas requisições
- Funções específicas para cada algoritmo de escalonamento
- Sistema de métricas desacoplado da lógica principal

#### Configuração de Scheduler
- Política definida em tempo de compilação via `-DSCHEDULER_DEFAULT`
- Permite comparação objetiva entre algoritmos
- Flexibilidade para testes específicos

#### Gerenciamento de Estado
- Rastreamento da posição da cabeça do disco
- Métricas em tempo real de performance
- Validação de parâmetros e estados

## Configurações

### Projeto A
Constantes principais em `ppos-core-aux.c`:
```c
#define QUANTUM_SIZE       20   // Quantum em ticks
#define TIMER_INTERVAL   1000   // Timer a cada 1ms
#define PRIORITY_ALPHA     -1   // Fator de aging
#define PRIORITY_MAX      -20   // Maior prioridade
#define PRIORITY_MIN       20   // Menor prioridade
```

### Projeto B
Constantes principais em `ppos_disk.c`:
```c
#define SCHEDULER_FCFS   1      // First Come, First Served
#define SCHEDULER_SSTF   2      // Shortest Seek Time First  
#define SCHEDULER_CSCAN  3      // Circular Scan
#define ERROR_INVALID   -1      // Código de erro padrão
```

## Utilitários do Makefile
```bash
# Limpeza
make clean                  # Remove tudo (preserva disk.dat)
make clean-bin              # Remove apenas executáveis
make clean-output           # Remove apenas resultados

# Verificação
make check-files            # Verifica arquivos necessários
make list-results           # Lista resultados disponíveis

# Ajuda
make help                   # Ajuda completa
```

## Observações Técnicas

### Projeto A
- Implementação focada nos requisitos específicos do Projeto A
- Compatível com a interface definida no núcleo do PingPongOS
- Utiliza apenas bibliotecas padrão Unix (`signal.h`, `sys/time.h`)
- Sistema de hooks para instrumentação e debugging

### Projeto B
- **IMPORTANTE**: Testes modificam o arquivo `disk.dat`
- Sempre faça backup antes de executar testes de disco
- Compilação é segura - apenas execução modifica dados
- Algoritmos implementados seguem especificações da literatura

## Configuração do Git
Recomenda-se adicionar ao `.gitignore`:
```gitignore
# Diretórios gerados durante compilação
bin/
output/

# Objetos compilados temporários
*.o

# Arquivos de backup
*~
*.bak
disk.dat.backup

# Arquivos temporários de teste
*.tmp
```

## Validação com Saídas de Referência
```bash
# Execute os testes e compare com saídas esperadas
make test-project-a
make test-project-b

# Compare resultado específico
diff output/pingpong-scheduler.txt expected-output/pingpong-scheduler.txt

# Visualize diferenças lado a lado
diff -y output/pingpong-scheduler.txt expected-output/pingpong-scheduler.txt
```

## Contexto Acadêmico

Este trabalho demonstra a implementação prática de:

### Projeto A
- Algoritmos de escalonamento de processos
- Sistemas de tempo real e controle de quantum
- Contabilização de recursos em sistemas operacionais
- Técnicas de sincronização e controle de concorrência

### Projeto B
- Gerenciamento de dispositivos de entrada/saída
- Algoritmos de escalonamento de disco
- Otimização de performance em sistemas de armazenamento
- Análise comparativa de algoritmos

## Performance Esperada (Projeto B)

### Características dos Algoritmos

**FCFS**: 
- Movimentação: Alta (sem otimização)
- Fairness: Excelente
- Complexidade: Baixa

**SSTF**:
- Movimentação: Baixa (otimizada)
- Fairness: Pode causar starvation
- Complexidade: Média

**CSCAN**:
- Movimentação: Média (balanceada)
- Fairness: Excelente
- Complexidade: Média

### Exemplo de Saída Final
```
Task 2 exit: execution time 15 ms, processor time 8 ms, 12 activations
```