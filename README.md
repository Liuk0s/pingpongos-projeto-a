# PingPongOS - Projeto A: Escalonador, Preempção e Contabilização

Este repositório contém a implementação das funcionalidades de escalonamento por prioridades, preempção por tempo e contabilização de recursos para o PingPongOS, um sistema operacional educacional desenvolvido para a disciplina de Sistemas Operacionais.

## Autores da Implementação

- **Iaritzza Bielinki**
- **Lucas Giovanni Thuler**

## Créditos e Contexto Acadêmico

- **Disciplina**: Sistemas Operacionais (EC) - UTFPR
- **Professor**: Marco Aurélio Wehrmeister
- **Sistema Base**: PingPongOS - criado pelo Prof. Carlos Maziero (DInf/UFPR)
- **Documentação oficial**: https://wiki.inf.ufpr.br/maziero/doku.php?id=so:pingpongos

*A autoria de todo o material base utilizado é do prof. Carlos Maziero (DInf/UFPR), que gentilmente o cedeu para aplicação nesta disciplina.*

## Sobre o Projeto

O PingPongOS é um sistema operacional educacional criado para demonstrar conceitos fundamentais de sistemas operacionais. Este projeto implementa três funcionalidades específicas solicitadas no **Projeto A**:

1. **Escalonador por Prioridades com Aging**
2. **Sistema de Preempção por Tempo**  
3. **Contabilização de Uso do Processador**

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
- **Hooks de sistema**: Instrumentação completa do ciclo de vida das tarefas

## Estrutura do Repositório

```
projeto/
├── entrega/                       # Arquivos para submissão acadêmica
│   ├── pa-ppos-core-aux.c
│   ├── pa-ppos-core-aux-modificacoes.txt
│   └── pa-ppos-data.h
├── educational/                   # Versão educacional
│   └── ppos-core-aux-tutorial.c
├── expected-output/               # Saídas de referência dos testes
│   ├── pingpong-dispatcher.txt
│   ├── pingpong-scheduler.txt
│   ├── pingpong-preempcao.txt
│   ├── pingpong-contab-prio.txt
│   └── pingpong-preempcao-stress.txt
├── ppos-core-aux.c               # Implementação principal
├── queue.o                       # Biblioteca de filas (fornecida)
├── ppos-all.o                   # Núcleo do PingPongOS (fornecido)
├── pingpong-*.c                 # Programas de teste (fornecidos)
├── bin/                         # Executáveis compilados (gerado)
├── output/                      # Resultados dos testes (gerado)
├── README.md
└── Makefile
```

### Arquivos de Entrega

Os seguintes arquivos estão formatados conforme os requisitos da disciplina:

- **`pa-ppos-core-aux.c`**: Implementação principal do Projeto A
- **`pa-ppos-core-aux-modificacoes.txt`**: Documentação das modificações realizadas  
- **`pa-ppos-data.h`**: Definições de dados específicas do projeto (se necessário)

O arquivo `pa-ppos-core-aux-modificacoes.txt` documenta:
- Funções implementadas
- Modificações realizadas no código base
- Decisões de design tomadas
- Explicação das escolhas técnicas

### Versões do Código

- **Versão de Entrega** (`entrega/`): Arquivos formatados para submissão acadêmica
- **Versão de Trabalho** (raiz): Código com comentários equilibrados usado durante desenvolvimento
- **Versão Tutorial** (`educational/`): Código com comentários extensivos para fins didáticos

*A versão tutorial contém explicações detalhadas de todos os conceitos implementados, ideal para estudo e referência educacional.*

### Saídas de Referência

A pasta `expected-output/` contém as saídas esperadas de cada teste para comparação e validação:
- Facilita verificação se implementação está correta
- Permite comparação com resultados obtidos (`output/` vs `expected-output/`)
- Útil para debug e análise de diferenças

## Arquivo Principal: `ppos-core-aux.c`

Este arquivo contém toda nossa implementação, organizada em seções:

- **Seção 1**: Constantes e configurações do sistema
- **Seção 2**: Implementação do escalonador (`scheduler()`)
- **Seção 3**: Função de tempo (`systime()`)
- **Seção 4**: Gerenciamento de prioridades (`task_setprio()`, `task_getprio()`)
- **Seção 5**: Sistema de preempção (`interrupt_handler()`, `timer_init()`)
- **Seção 6-7**: Hooks de inicialização e gerenciamento de tarefas

## Como Usar

### Pré-requisitos
- GCC (GNU Compiler Collection)
- Make
- Sistema Unix/Linux
- Arquivos base do PingPongOS (`queue.o`, `ppos-all.o`, programas de teste)

### Configuração Inicial
```bash
# Clone o repositório
git clone [seu-repositório]
cd projeto

# Os arquivos de trabalho já estão na raiz
# Certifique-se de ter os arquivos base do PingPongOS:
# - queue.o, ppos-all.o (objetos fornecidos)
# - pingpong-*.c (programas de teste)
```

### Compilação
```bash
# Compila todos os testes
make all

# Compila teste específico
make pingpong-scheduler
```

### Execução com Resultados Salvos
```bash
# Executa todos os testes e salva em output/
make test-all

# Executa teste específico
make test-scheduler
```

### Execução na Tela
```bash
# Ver saída diretamente na tela
make test-all-screen
make run-scheduler
```

### Utilitários
```bash
# Ver resultados salvos
make show-output

# Debug (compila com -DDEBUG -DDEBUG02)
make debug

# Limpeza
make clean
```

### Validação com Saídas de Referência
```bash
# Execute os testes e compare com saídas esperadas
make test-all

# Compare resultado específico
diff output/pingpong-scheduler.txt expected-output/pingpong-scheduler.txt

# Visualize diferenças lado a lado
diff -y output/pingpong-scheduler.txt expected-output/pingpong-scheduler.txt
```

## Testes Disponíveis

Os seguintes programas de teste (fornecidos pela disciplina) validam nossa implementação:

- **`pingpong-dispatcher`**: Testa funcionamento básico do dispatcher
- **`pingpong-scheduler`**: Valida escalonador por prioridades e aging
- **`pingpong-preempcao`**: Testa sistema de preempção por tempo
- **`pingpong-contab-prio`**: Valida contabilização e ajuste de prioridades  
- **`pingpong-preempcao-stress`**: Teste de stress da preempção

## Decisões de Implementação

### Controle de Preempção
- Todas as operações críticas são protegidas com `PPOS_PREEMPT_DISABLE/ENABLE`
- Garante atomicidade durante modificação de estruturas do sistema
- Evita condições de corrida no escalonador e gerenciamento de tarefas

### Classificação de Tarefas
- **Tarefas de sistema** (ID ≤ 1): main, dispatcher
- **Tarefas de usuário** (ID > 1): participam do aging e sofrem preempção

### Aging e Prioridades
- Fator alpha = -1 (melhora prioridade das não-escolhidas)
- Limites: -20 (maior prioridade) a +20 (menor prioridade)
- Reset automático da tarefa escolhida para prioridade estática

## Configurações

Constantes principais em `ppos-core-aux.c`:

```c
#define QUANTUM_SIZE       20    // Quantum em ticks
#define TIMER_INTERVAL   1000    // Timer a cada 1ms  
#define PRIORITY_ALPHA     -1    // Fator de aging
#define PRIORITY_MAX      -20    // Maior prioridade
#define PRIORITY_MIN       20    // Menor prioridade
```

## Saída Esperada

Cada teste produz saída específica mostrando:
- Alternância entre tarefas conforme prioridades
- Aplicação correta do aging
- Funcionamento da preempção por tempo
- Estatísticas finais de cada tarefa

Exemplo de estatística final:
```
Task 2 exit: execution time 15 ms, processor time 8 ms, 12 activations
```

## Observações Técnicas

- Implementação focada nos requisitos específicos do Projeto A
- Compatível com a interface definida no núcleo do PingPongOS
- Utiliza apenas bibliotecas padrão Unix (`signal.h`, `sys/time.h`)
- Sistema de hooks completo para instrumentação e debugging


## Configuração do Git

Recomenda-se adicionar ao `.gitignore`:
```
# Diretórios gerados durante compilação
bin/
output/

# Objetos compilados temporários
*.o

# Arquivos de backup
*~
*.bak
```

## Contexto Acadêmico

Este trabalho demonstra a implementação prática de:
- Algoritmos de escalonamento de processos
- Sistemas de tempo real e controle de quantum
- Contabilização de recursos em sistemas operacionais
- Técnicas de sincronização e controle de concorrência