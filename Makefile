# ============================================================================
# Makefile Unificado para PingPongOS - Projetos A e B
# Compila testes para escalonador (A) e disco (B) automaticamente
# ============================================================================

# Configurações do compilador
CC = gcc
CFLAGS = -Wall -std=c99 -D_XOPEN_SOURCE=600
LDFLAGS = -lrt

# Flags opcionais (descomente conforme necessário)
# CFLAGS += -g          # Informações de debug para gdb
# CFLAGS += -O2         # Otimização

# Diretórios
BIN_DIR = bin
OUTPUT_DIR = output

# ============================================================================
# DETECÇÃO AUTOMÁTICA DE PROJETO
# ============================================================================

# Detecta qual projeto está disponível baseado nos arquivos presentes
PROJECT_A_FILES = pingpong-contab-prio.c pingpong-dispatcher.c pingpong-preempcao.c pingpong-scheduler.c
PROJECT_B_FILES = pingpong-disco1.c pingpong-disco2.c ppos_disk.c disk-driver.o

# Verifica se arquivos do Projeto A existem
PROJECT_A_AVAILABLE := $(shell \
	for file in $(PROJECT_A_FILES); do \
		if [ ! -f "$$file" ]; then \
			echo "0"; \
			exit; \
		fi; \
	done; \
	echo "1" \
)

# Verifica se arquivos do Projeto B existem
PROJECT_B_AVAILABLE := $(shell \
	for file in $(PROJECT_B_FILES); do \
		if [ ! -f "$$file" ]; then \
			echo "0"; \
			exit; \
		fi; \
	done; \
	echo "1" \
)

# Define projeto padrão baseado na disponibilidade
ifeq ($(PROJECT_B_AVAILABLE),1)
	DEFAULT_PROJECT = B
else ifeq ($(PROJECT_A_AVAILABLE),1)
	DEFAULT_PROJECT = A
else
	DEFAULT_PROJECT = NONE
endif

# Permite override via linha de comando: make PROJECT=A ou make PROJECT=B
PROJECT ?= $(DEFAULT_PROJECT)

# ============================================================================
# CONFIGURAÇÕES POR PROJETO
# ============================================================================

# Projeto A - Escalonador e Preempção
ifeq ($(PROJECT),A)
	USER_SOURCES = ppos-core-aux.c
	SYSTEM_OBJECTS = queue.o ppos-all.o
	TEST_SOURCES = pingpong-contab-prio.c pingpong-dispatcher.c pingpong-preempcao.c \
	               pingpong-preempcao-stress.c pingpong-scheduler.c
	TEST_NAMES = pingpong-contab-prio pingpong-dispatcher pingpong-preempcao \
	             pingpong-preempcao-stress pingpong-scheduler
	PROJECT_TITLE = "PROJETO A - Escalonador e Preempção"
endif

# Projeto B - Gerenciador de Disco
ifeq ($(PROJECT),B)
	USER_SOURCES = ppos-core-aux.c ppos_disk.c
	SYSTEM_OBJECTS = disk-driver.o queue.o ppos-all.o
	TEST_SOURCES = pingpong-disco1.c pingpong-disco2.c
	SCHEDULERS = fcfs sstf cscan
	PROJECT_TITLE = "PROJETO B - Gerenciador de Disco"
endif

# Objetos compilados
USER_OBJECTS = $(USER_SOURCES:.c=.o)
ALL_OBJECTS = $(USER_OBJECTS) $(SYSTEM_OBJECTS)

# ============================================================================
# ALVOS PRINCIPAIS
# ============================================================================

# Alvo padrão - detecta projeto e compila
all: detect-project
ifeq ($(PROJECT),A)
	@$(MAKE) --no-print-directory all-project-a
else ifeq ($(PROJECT),B)
	@$(MAKE) --no-print-directory all-project-b
else
	@$(MAKE) --no-print-directory show-project-help
endif

# Detecta e mostra qual projeto será compilado
detect-project:
	@echo "========================================="
	@echo "DETECÇÃO AUTOMÁTICA DE PROJETO"
	@echo "========================================="
	@echo "Projeto A disponível: $(if $(filter 1,$(PROJECT_A_AVAILABLE)),SIM,NÃO)"
	@echo "Projeto B disponível: $(if $(filter 1,$(PROJECT_B_AVAILABLE)),SIM,NÃO)"
	@echo "Projeto selecionado: $(PROJECT)"
ifeq ($(PROJECT),A)
	@echo "Compilando: $(PROJECT_TITLE)"
else ifeq ($(PROJECT),B)
	@echo "Compilando: $(PROJECT_TITLE)"
else
	@echo "ERRO: Nenhum projeto detectado ou selecionado!"
endif
	@echo ""

# Força compilação do Projeto A
project-a:
	@$(MAKE) PROJECT=A all-project-a

# Força compilação do Projeto B  
project-b:
	@$(MAKE) PROJECT=B all-project-b

# ============================================================================
# PROJETO A - ESCALONADOR E PREEMPÇÃO
# ============================================================================

all-project-a: $(BIN_DIR) $(addprefix $(BIN_DIR)/,$(TEST_NAMES))
	@echo "========================================="
	@echo "PROJETO A - Compilação Concluída!"
	@echo "========================================="
	@echo "Executáveis criados em $(BIN_DIR)/:"
	@for test in $(TEST_NAMES); do \
		echo "  $$test"; \
	done
	@echo ""
	@echo "Use 'make test-project-a' para executar todos os testes."

# Compilação individual dos testes do Projeto A
$(BIN_DIR)/pingpong-contab-prio: pingpong-contab-prio.c $(ALL_OBJECTS) | $(BIN_DIR)
	@echo "Compilando teste de contabilização e prioridades..."
	$(CC) $(CFLAGS) -o $@ $< $(ALL_OBJECTS) $(LDFLAGS)

$(BIN_DIR)/pingpong-dispatcher: pingpong-dispatcher.c $(ALL_OBJECTS) | $(BIN_DIR)
	@echo "Compilando teste do dispatcher..."
	$(CC) $(CFLAGS) -o $@ $< $(ALL_OBJECTS) $(LDFLAGS)

$(BIN_DIR)/pingpong-preempcao: pingpong-preempcao.c $(ALL_OBJECTS) | $(BIN_DIR)
	@echo "Compilando teste de preempção..."
	$(CC) $(CFLAGS) -o $@ $< $(ALL_OBJECTS) $(LDFLAGS)

$(BIN_DIR)/pingpong-preempcao-stress: pingpong-preempcao-stress.c $(ALL_OBJECTS) | $(BIN_DIR)
	@echo "Compilando teste de stress de preempção..."
	$(CC) $(CFLAGS) -o $@ $< $(ALL_OBJECTS) $(LDFLAGS)

$(BIN_DIR)/pingpong-scheduler: pingpong-scheduler.c $(ALL_OBJECTS) | $(BIN_DIR)
	@echo "Compilando teste do escalonador..."
	$(CC) $(CFLAGS) -o $@ $< $(ALL_OBJECTS) $(LDFLAGS)

# Testes do Projeto A
test-project-a: all-project-a $(OUTPUT_DIR)
	@echo "========================================="
	@echo "EXECUTANDO TESTES DO PROJETO A"
	@echo "========================================="
	@for test in $(TEST_NAMES); do \
		echo "Executando $$test..."; \
		-$(BIN_DIR)/$$test > $(OUTPUT_DIR)/$$test.txt 2>&1; \
		echo "Resultado salvo em $(OUTPUT_DIR)/$$test.txt"; \
		echo ""; \
	done
	@echo "Todos os testes do Projeto A executados!"

# ============================================================================
# PROJETO B - GERENCIADOR DE DISCO
# ============================================================================

all-project-b: $(BIN_DIR) $(addprefix $(BIN_DIR)/pingpong-disco1-,$(SCHEDULERS)) $(addprefix $(BIN_DIR)/pingpong-disco2-,$(SCHEDULERS))
	@echo "========================================="
	@echo "PROJETO B - Compilação Concluída!"
	@echo "========================================="
	@echo "Executáveis criados em $(BIN_DIR)/:"
	@echo ""
	@echo "PINGPONG-DISCO1 (teste básico):"
	@for sched in $(SCHEDULERS); do \
		echo "  pingpong-disco1-$sched   - $(echo $sched | tr '[:lower:]' '[:upper:]')"; \
	done
	@echo ""
	@echo "PINGPONG-DISCO2 (teste com múltiplas tarefas):"
	@for sched in $(SCHEDULERS); do \
		echo "  pingpong-disco2-$sched   - $(echo $sched | tr '[:lower:]' '[:upper:]')"; \
	done
	@echo ""
	@echo "   Para executar testes:"
	@echo "   make backup-disk         # Crie um backup primeiro!"
	@echo "   make disco1-fcfs         # Teste individual"
	@echo "   make test-project-b      # Todos os testes"

# Compilação dos testes de disco com schedulers específicos
$(BIN_DIR)/pingpong-disco1-fcfs: pingpong-disco1.c $(ALL_OBJECTS) | $(BIN_DIR)
	@echo "Compilando disco1 com scheduler FCFS..."
	$(CC) $(CFLAGS) -DSCHEDULER_DEFAULT=SCHEDULER_FCFS -o $@ $< $(ALL_OBJECTS) $(LDFLAGS)

$(BIN_DIR)/pingpong-disco1-sstf: pingpong-disco1.c $(ALL_OBJECTS) | $(BIN_DIR)
	@echo "Compilando disco1 com scheduler SSTF..."
	$(CC) $(CFLAGS) -DSCHEDULER_DEFAULT=SCHEDULER_SSTF -o $@ $< $(ALL_OBJECTS) $(LDFLAGS)

$(BIN_DIR)/pingpong-disco1-cscan: pingpong-disco1.c $(ALL_OBJECTS) | $(BIN_DIR)
	@echo "Compilando disco1 com scheduler CSCAN..."
	$(CC) $(CFLAGS) -DSCHEDULER_DEFAULT=SCHEDULER_CSCAN -o $@ $< $(ALL_OBJECTS) $(LDFLAGS)

$(BIN_DIR)/pingpong-disco2-fcfs: pingpong-disco2.c $(ALL_OBJECTS) | $(BIN_DIR)
	@echo "Compilando disco2 com scheduler FCFS..."
	$(CC) $(CFLAGS) -DSCHEDULER_DEFAULT=SCHEDULER_FCFS -o $@ $< $(ALL_OBJECTS) $(LDFLAGS)

$(BIN_DIR)/pingpong-disco2-sstf: pingpong-disco2.c $(ALL_OBJECTS) | $(BIN_DIR)
	@echo "Compilando disco2 com scheduler SSTF..."
	$(CC) $(CFLAGS) -DSCHEDULER_DEFAULT=SCHEDULER_SSTF -o $@ $< $(ALL_OBJECTS) $(LDFLAGS)

$(BIN_DIR)/pingpong-disco2-cscan: pingpong-disco2.c $(ALL_OBJECTS) | $(BIN_DIR)
	@echo "Compilando disco2 com scheduler CSCAN..."
	$(CC) $(CFLAGS) -DSCHEDULER_DEFAULT=SCHEDULER_CSCAN -o $@ $< $(ALL_OBJECTS) $(LDFLAGS)

# Aliases para compilação parcial do Projeto B
disco1-all: $(addprefix $(BIN_DIR)/pingpong-disco1-,$(SCHEDULERS))
	@echo "Compilação do disco1 (todos os schedulers) concluída!"

disco2-all: $(addprefix $(BIN_DIR)/pingpong-disco2-,$(SCHEDULERS))
	@echo "Compilação do disco2 (todos os schedulers) concluída!"

scheduler-fcfs: $(BIN_DIR)/pingpong-disco1-fcfs $(BIN_DIR)/pingpong-disco2-fcfs
	@echo "Compilação com FCFS concluída!"

scheduler-sstf: $(BIN_DIR)/pingpong-disco1-sstf $(BIN_DIR)/pingpong-disco2-sstf
	@echo "Compilação com SSTF concluída!"

scheduler-cscan: $(BIN_DIR)/pingpong-disco1-cscan $(BIN_DIR)/pingpong-disco2-cscan
	@echo "Compilação com CSCAN concluída!"

# Testes do Projeto B
test-project-b: all-project-b $(OUTPUT_DIR)
	@echo "========================================="
	@echo "   AVISO IMPORTANTE"
	@echo "========================================="
	@echo "Os testes de disco modificam o arquivo disk.dat!"
	@echo "Recomenda-se fazer backup antes de continuar."
	@echo ""
	@read -p "Pressione Enter para continuar ou Ctrl+C para cancelar..."
	@echo ""
	@echo "========================================="
	@echo "EXECUTANDO TESTES DO PROJETO B"
	@echo "========================================="
	@echo ""
	@$(MAKE) --no-print-directory test-disco1-all-schedulers
	@echo ""
	@$(MAKE) --no-print-directory test-disco2-all-schedulers
	@echo ""
	@$(MAKE) --no-print-directory compare-disk-results
	@echo ""
	@echo "Todos os testes do Projeto B executados!"
	@echo "Veja resumo comparativo em $(OUTPUT_DIR)/resumo-comparativo.txt"

test-disco1-all-schedulers: $(OUTPUT_DIR)
	@echo "   Testando disco1 com todos os schedulers (modifica disk.dat)..."
	@for sched in $(SCHEDULERS); do \
		echo "  Executando disco1 com $sched..."; \
		-$(BIN_DIR)/pingpong-disco1-$sched > $(OUTPUT_DIR)/disco1-$sched.txt 2>&1; \
	done
	@echo "Disco1 - Todos os schedulers testados!"

test-disco2-all-schedulers: $(OUTPUT_DIR)
	@echo "   Testando disco2 com todos os schedulers (modifica disk.dat)..."
	@for sched in $(SCHEDULERS); do \
		echo "  Executando disco2 com $sched..."; \
		-$(BIN_DIR)/pingpong-disco2-$sched > $(OUTPUT_DIR)/disco2-$sched.txt 2>&1; \
	done
	@echo "Disco2 - Todos os schedulers testados!"

# ============================================================================
# COMPILAÇÃO DE OBJETOS
# ============================================================================

# Cria diretórios se não existirem
$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(OUTPUT_DIR):
	@mkdir -p $(OUTPUT_DIR)

# Compila objetos do usuário
%.o: %.c
	@echo "Compilando objeto: $@"
	$(CC) $(CFLAGS) -c $< -o $@

# ============================================================================
# TESTES INDIVIDUAIS POR SCHEDULER (PROJETO B)
# ============================================================================

# AVISO: Os testes de disco modificam o arquivo disk.dat!
# Recomenda-se fazer backup antes de executar os testes.

test-fcfs-only: $(BIN_DIR)/pingpong-disco1-fcfs $(BIN_DIR)/pingpong-disco2-fcfs $(OUTPUT_DIR)
	@echo "   AVISO: Este teste modificará o arquivo disk.dat!"
	@echo "Testando apenas scheduler FCFS..."
	-$(BIN_DIR)/pingpong-disco1-fcfs > $(OUTPUT_DIR)/disco1-fcfs.txt 2>&1
	-$(BIN_DIR)/pingpong-disco2-fcfs > $(OUTPUT_DIR)/disco2-fcfs.txt 2>&1
	@echo "Testes FCFS concluídos!"

test-sstf-only: $(BIN_DIR)/pingpong-disco1-sstf $(BIN_DIR)/pingpong-disco2-sstf $(OUTPUT_DIR)
	@echo "   AVISO: Este teste modificará o arquivo disk.dat!"
	@echo "Testando apenas scheduler SSTF..."
	-$(BIN_DIR)/pingpong-disco1-sstf > $(OUTPUT_DIR)/disco1-sstf.txt 2>&1
	-$(BIN_DIR)/pingpong-disco2-sstf > $(OUTPUT_DIR)/disco2-sstf.txt 2>&1
	@echo "Testes SSTF concluídos!"

test-cscan-only: $(BIN_DIR)/pingpong-disco1-cscan $(BIN_DIR)/pingpong-disco2-cscan $(OUTPUT_DIR)
	@echo "   AVISO: Este teste modificará o arquivo disk.dat!"
	@echo "Testando apenas scheduler CSCAN..."
	-$(BIN_DIR)/pingpong-disco1-cscan > $(OUTPUT_DIR)/disco1-cscan.txt 2>&1
	-$(BIN_DIR)/pingpong-disco2-cscan > $(OUTPUT_DIR)/disco2-cscan.txt 2>&1
	@echo "Testes CSCAN concluídos!"

# ============================================================================
# TESTES INDIVIDUAIS ESPECÍFICOS (PROJETO B)
# ============================================================================

# DISCO1 - Testes individuais com cada scheduler
disco1-fcfs: $(BIN_DIR)/pingpong-disco1-fcfs $(OUTPUT_DIR)
	@echo "   AVISO: Este teste modificará o arquivo disk.dat!"
	@echo "Executando disco1 com FCFS..."
	-$(BIN_DIR)/pingpong-disco1-fcfs > $(OUTPUT_DIR)/disco1-fcfs.txt 2>&1
	@echo "Resultado salvo em $(OUTPUT_DIR)/disco1-fcfs.txt"

disco1-sstf: $(BIN_DIR)/pingpong-disco1-sstf $(OUTPUT_DIR)
	@echo "   AVISO: Este teste modificará o arquivo disk.dat!"
	@echo "Executando disco1 com SSTF..."
	-$(BIN_DIR)/pingpong-disco1-sstf > $(OUTPUT_DIR)/disco1-sstf.txt 2>&1
	@echo "Resultado salvo em $(OUTPUT_DIR)/disco1-sstf.txt"

disco1-cscan: $(BIN_DIR)/pingpong-disco1-cscan $(OUTPUT_DIR)
	@echo "   AVISO: Este teste modificará o arquivo disk.dat!"
	@echo "Executando disco1 com CSCAN..."
	-$(BIN_DIR)/pingpong-disco1-cscan > $(OUTPUT_DIR)/disco1-cscan.txt 2>&1
	@echo "Resultado salvo em $(OUTPUT_DIR)/disco1-cscan.txt"

# DISCO2 - Testes individuais com cada scheduler
disco2-fcfs: $(BIN_DIR)/pingpong-disco2-fcfs $(OUTPUT_DIR)
	@echo "   AVISO: Este teste modificará o arquivo disk.dat!"
	@echo "Executando disco2 com FCFS..."
	-$(BIN_DIR)/pingpong-disco2-fcfs > $(OUTPUT_DIR)/disco2-fcfs.txt 2>&1
	@echo "Resultado salvo em $(OUTPUT_DIR)/disco2-fcfs.txt"

disco2-sstf: $(BIN_DIR)/pingpong-disco2-sstf $(OUTPUT_DIR)
	@echo "   AVISO: Este teste modificará o arquivo disk.dat!"
	@echo "Executando disco2 com SSTF..."
	-$(BIN_DIR)/pingpong-disco2-sstf > $(OUTPUT_DIR)/disco2-sstf.txt 2>&1
	@echo "Resultado salvo em $(OUTPUT_DIR)/disco2-sstf.txt"

disco2-cscan: $(BIN_DIR)/pingpong-disco2-cscan $(OUTPUT_DIR)
	@echo "   AVISO: Este teste modificará o arquivo disk.dat!"
	@echo "Executando disco2 com CSCAN..."
	-$(BIN_DIR)/pingpong-disco2-cscan > $(OUTPUT_DIR)/disco2-cscan.txt 2>&1
	@echo "Resultado salvo em $(OUTPUT_DIR)/disco2-cscan.txt"

# Todos os testes do disco1
disco1-all-tests: disco1-fcfs disco1-sstf disco1-cscan
	@echo "Todos os testes do disco1 executados!"

# Todos os testes do disco2  
disco2-all-tests: disco2-fcfs disco2-sstf disco2-cscan
	@echo "Todos os testes do disco2 executados!"

# ============================================================================
# EXECUÇÃO COM SAÍDA NA TELA (PARA DEBUG)
# ============================================================================

# Projeto A - sem problemas de modificação de arquivos
run-scheduler: $(BIN_DIR)/pingpong-scheduler
	@echo "Executando teste do escalonador (saída na tela)..."
	$(BIN_DIR)/pingpong-scheduler

run-preempcao: $(BIN_DIR)/pingpong-preempcao
	@echo "Executando teste de preempção (saída na tela)..."
	$(BIN_DIR)/pingpong-preempcao

run-contab: $(BIN_DIR)/pingpong-contab-prio
	@echo "Executando teste de contabilização (saída na tela)..."
	$(BIN_DIR)/pingpong-contab-prio

# Projeto B - AVISO: modifica disk.dat!
run-disco1-fcfs: $(BIN_DIR)/pingpong-disco1-fcfs
	@echo "   AVISO: Este comando modificará o arquivo disk.dat!"
	@echo "Executando disco1 com FCFS (saída na tela)..."
	$(BIN_DIR)/pingpong-disco1-fcfs

run-disco1-sstf: $(BIN_DIR)/pingpong-disco1-sstf
	@echo "   AVISO: Este comando modificará o arquivo disk.dat!"
	@echo "Executando disco1 com SSTF (saída na tela)..."
	$(BIN_DIR)/pingpong-disco1-sstf

run-disco1-cscan: $(BIN_DIR)/pingpong-disco1-cscan
	@echo "   AVISO: Este comando modificará o arquivo disk.dat!"
	@echo "Executando disco1 com CSCAN (saída na tela)..."
	$(BIN_DIR)/pingpong-disco1-cscan

run-disco2-fcfs: $(BIN_DIR)/pingpong-disco2-fcfs
	@echo "   AVISO: Este comando modificará o arquivo disk.dat!"
	@echo "Executando disco2 com FCFS (saída na tela)..."
	$(BIN_DIR)/pingpong-disco2-fcfs

run-disco2-sstf: $(BIN_DIR)/pingpong-disco2-sstf
	@echo "   AVISO: Este comando modificará o arquivo disk.dat!"
	@echo "Executando disco2 com SSTF (saída na tela)..."
	$(BIN_DIR)/pingpong-disco2-sstf

run-disco2-cscan: $(BIN_DIR)/pingpong-disco2-cscan
	@echo "   AVISO: Este comando modificará o arquivo disk.dat!"
	@echo "Executando disco2 com CSCAN (saída na tela)..."
	$(BIN_DIR)/pingpong-disco2-cscan

# ============================================================================
# ANÁLISE COMPARATIVA (PROJETO B)
# ============================================================================

compare-disk-results: $(OUTPUT_DIR)
	@echo "Gerando relatório comparativo dos schedulers..."
	@echo "# RELATÓRIO COMPARATIVO DE PERFORMANCE DOS SCHEDULERS" > $(OUTPUT_DIR)/resumo-comparativo.txt
	@echo "# Gerado em: $$(date)" >> $(OUTPUT_DIR)/resumo-comparativo.txt
	@echo "" >> $(OUTPUT_DIR)/resumo-comparativo.txt
	@echo "## DISCO1 (Teste Básico)" >> $(OUTPUT_DIR)/resumo-comparativo.txt
	@echo "================================" >> $(OUTPUT_DIR)/resumo-comparativo.txt
	@for scheduler in $(SCHEDULERS); do \
		echo "" >> $(OUTPUT_DIR)/resumo-comparativo.txt; \
		echo "### SCHEDULER: $$scheduler" >> $(OUTPUT_DIR)/resumo-comparativo.txt; \
		echo "----------------------------" >> $(OUTPUT_DIR)/resumo-comparativo.txt; \
		if [ -f "$(OUTPUT_DIR)/disco1-$$scheduler.txt" ]; then \
			grep -E "(Política ativa|Requisições processadas|Movimentação total|Movimentação média|Tempo total)" $(OUTPUT_DIR)/disco1-$$scheduler.txt >> $(OUTPUT_DIR)/resumo-comparativo.txt 2>/dev/null || true; \
		fi; \
	done
	@echo "" >> $(OUTPUT_DIR)/resumo-comparativo.txt
	@echo "## DISCO2 (Teste com Múltiplas Tarefas)" >> $(OUTPUT_DIR)/resumo-comparativo.txt
	@echo "======================================" >> $(OUTPUT_DIR)/resumo-comparativo.txt
	@for scheduler in $(SCHEDULERS); do \
		echo "" >> $(OUTPUT_DIR)/resumo-comparativo.txt; \
		echo "### SCHEDULER: $$scheduler" >> $(OUTPUT_DIR)/resumo-comparativo.txt; \
		echo "----------------------------" >> $(OUTPUT_DIR)/resumo-comparativo.txt; \
		if [ -f "$(OUTPUT_DIR)/disco2-$$scheduler.txt" ]; then \
			grep -E "(Política ativa|Requisições processadas|Movimentação total|Movimentação média|Tempo total)" $(OUTPUT_DIR)/disco2-$$scheduler.txt >> $(OUTPUT_DIR)/resumo-comparativo.txt 2>/dev/null || true; \
		fi; \
	done
	@echo "Relatório comparativo salvo em $(OUTPUT_DIR)/resumo-comparativo.txt"

extract-disk-metrics: $(OUTPUT_DIR)
	@echo "SCHEDULER,TESTE,REQUISICOES,MOVIMENTO_TOTAL,MOVIMENTO_MEDIO,TEMPO_MS" > $(OUTPUT_DIR)/metricas.csv
	@for scheduler in $(SCHEDULERS); do \
		for teste in disco1 disco2; do \
			if [ -f "$(OUTPUT_DIR)/$$teste-$$scheduler.txt" ]; then \
				req=$$(grep "Requisições processadas" $(OUTPUT_DIR)/$$teste-$$scheduler.txt | grep -o '[0-9]*' 2>/dev/null || echo "0"); \
				mov_total=$$(grep "Movimentação total" $(OUTPUT_DIR)/$$teste-$$scheduler.txt | grep -o '[0-9]*' 2>/dev/null || echo "0"); \
				mov_medio=$$(grep "Movimentação média" $(OUTPUT_DIR)/$$teste-$$scheduler.txt | grep -o '[0-9]*\.[0-9]*' 2>/dev/null || echo "0.00"); \
				tempo=$$(grep "Tempo total" $(OUTPUT_DIR)/$$teste-$$scheduler.txt | grep -o '[0-9]*' 2>/dev/null || echo "0"); \
				echo "$$scheduler,$$teste,$$req,$$mov_total,$$mov_medio,$$tempo" >> $(OUTPUT_DIR)/metricas.csv; \
			fi; \
		done; \
	done
	@echo "Métricas extraídas para $(OUTPUT_DIR)/metricas.csv"

# ============================================================================
# UTILITÁRIOS
# ============================================================================

# Backup e restauração do disk.dat (importante para testes)
backup-disk:
	@if [ -f "disk.dat" ]; then \
		cp disk.dat disk.dat.backup; \
		echo "Backup criado: disk.dat.backup"; \
	else \
		echo "Arquivo disk.dat não encontrado."; \
	fi

restore-disk:
	@if [ -f "disk.dat.backup" ]; then \
		cp disk.dat.backup disk.dat; \
		echo "Arquivo disk.dat restaurado do backup."; \
	else \
		echo "Backup disk.dat.backup não encontrado."; \
	fi

# Remove todos os arquivos compilados
clean:
	@echo "Limpando arquivos compilados..."
	rm -rf $(BIN_DIR) $(OUTPUT_DIR)
	rm -f $(USER_OBJECTS)
	@echo "Limpeza concluída!"
	@echo "NOTA: disk.dat e backups preservados."

# Remove apenas executáveis
clean-bin:
	@echo "Removendo executáveis..."
	rm -rf $(BIN_DIR)

# Remove apenas resultados
clean-output:
	@echo "Removendo resultados dos testes..."
	rm -rf $(OUTPUT_DIR)

# Força recompilação completa
rebuild: clean all

# Lista arquivos de resultado disponíveis
list-results:
	@if [ -d "$(OUTPUT_DIR)" ]; then \
		echo "Resultados disponíveis em $(OUTPUT_DIR)/:"; \
		ls -la $(OUTPUT_DIR)/ 2>/dev/null || echo "Pasta vazia."; \
	else \
		echo "Pasta $(OUTPUT_DIR)/ não existe. Execute 'make test-project-a' ou 'make test-project-b' primeiro."; \
	fi

# Verifica arquivos necessários
check-files:
	@echo "========================================="
	@echo "VERIFICAÇÃO DE ARQUIVOS"
	@echo "========================================="
	@echo "PROJETO A:"
	@for file in $(PROJECT_A_FILES); do \
		if [ -f "$$file" ]; then \
			echo "  ✓ $$file"; \
		else \
			echo "  ✗ $$file"; \
		fi; \
	done
	@echo ""
	@echo "PROJETO B:"
	@for file in $(PROJECT_B_FILES); do \
		if [ -f "$$file" ]; then \
			echo "  ✓ $$file"; \
		else \
			echo "  ✗ $$file"; \
		fi; \
	done

# Mostra qual projeto está ativo e como mudar
show-project-status:
	@echo "========================================="
	@echo "STATUS DOS PROJETOS"
	@echo "========================================="
	@echo "Projeto A disponível: $(if $(filter 1,$(PROJECT_A_AVAILABLE)),SIM,NÃO)"
	@echo "Projeto B disponível: $(if $(filter 1,$(PROJECT_B_AVAILABLE)),SIM,NÃO)"
	@echo "Projeto ativo: $(PROJECT)"
	@echo ""
	@echo "Para forçar um projeto específico:"
	@echo "  make PROJECT=A all      # Força Projeto A"
	@echo "  make PROJECT=B all      # Força Projeto B"
	@echo "  make project-a          # Compila Projeto A"
	@echo "  make project-b          # Compila Projeto B"

# Ajuda quando nenhum projeto detectado
show-project-help:
	@echo "========================================="
	@echo "ERRO: NENHUM PROJETO DETECTADO"
	@echo "========================================="
	@echo ""
	@echo "Para usar este makefile, você precisa ter os arquivos de um dos projetos:"
	@echo ""
	@echo "PROJETO A (Escalonador e Preempção):"
	@for file in $(PROJECT_A_FILES); do \
		echo "  - $$file"; \
	done
	@echo ""
	@echo "PROJETO B (Gerenciador de Disco):"
	@for file in $(PROJECT_B_FILES); do \
		echo "  - $$file"; \
	done
	@echo ""
	@echo "Verifique se os arquivos estão no diretório atual e tente novamente."

# Ajuda completa
help:
	@echo "========================================="
	@echo "MAKEFILE UNIFICADO PINGPONGOS"
	@echo "========================================="
	@echo ""
	@echo "DETECÇÃO AUTOMÁTICA:"
	@echo "  all                 - Detecta projeto e compila automaticamente"
	@echo "  detect-project      - Mostra qual projeto foi detectado"
	@echo "  show-project-status - Status detalhado dos projetos"
	@echo ""
	@echo "FORÇAR PROJETO ESPECÍFICO:"
	@echo "  project-a           - Força compilação do Projeto A"
	@echo "  project-b           - Força compilação do Projeto B"
	@echo "  PROJECT=A all       - Compila Projeto A via variável"
	@echo "  PROJECT=B all       - Compila Projeto B via variável"
	@echo ""
	@echo "TESTES COMPLETOS (   Projeto B modifica disk.dat):"
	@echo "  test-project-a      - Executa todos os testes do Projeto A"
	@echo "  test-project-b      - Executa todos os testes do Projeto B"
	@echo ""
	@echo "PROJETO A - TESTES INDIVIDUAIS:"
	@echo "  run-scheduler       - Teste do escalonador (tela)"
	@echo "  run-preempcao       - Teste de preempção (tela)"
	@echo "  run-contab          - Teste de contabilização (tela)"
	@echo ""
	@echo "PROJETO B - COMPILAÇÃO APENAS (seguro, não modifica disk.dat):"
	@echo "  disco1-all          - Compila disco1 (todos schedulers)"
	@echo "  disco2-all          - Compila disco2 (todos schedulers)"
	@echo "  scheduler-fcfs      - Compila apenas FCFS"
	@echo "  scheduler-sstf      - Compila apenas SSTF"
	@echo "  scheduler-cscan     - Compila apenas CSCAN"
	@echo ""
	@echo "PROJETO B - TESTES INDIVIDUAIS (   modifica disk.dat):"
	@echo "  disco1-fcfs         - Testa disco1 com FCFS"
	@echo "  disco1-sstf         - Testa disco1 com SSTF" 
	@echo "  disco1-cscan        - Testa disco1 com CSCAN"
	@echo "  disco2-fcfs         - Testa disco2 com FCFS"
	@echo "  disco2-sstf         - Testa disco2 com SSTF"
	@echo "  disco2-cscan        - Testa disco2 com CSCAN"
	@echo ""
	@echo "PROJETO B - TESTES AGRUPADOS (   modifica disk.dat):"
	@echo "  disco1-all-tests    - Todos os testes do disco1"
	@echo "  disco2-all-tests    - Todos os testes do disco2"
	@echo "  test-fcfs-only      - Testa apenas FCFS (disco1+disco2)"
	@echo "  test-sstf-only      - Testa apenas SSTF (disco1+disco2)"
	@echo "  test-cscan-only     - Testa apenas CSCAN (disco1+disco2)"
	@echo ""
	@echo "PROJETO B - EXECUÇÃO DIRETA NA TELA (   modifica disk.dat):"
	@echo "  run-disco1-fcfs     - Disco1 FCFS (tela)"
	@echo "  run-disco1-sstf     - Disco1 SSTF (tela)"
	@echo "  run-disco1-cscan    - Disco1 CSCAN (tela)"
	@echo "  run-disco2-fcfs     - Disco2 FCFS (tela)"
	@echo "  run-disco2-sstf     - Disco2 SSTF (tela)"
	@echo "  run-disco2-cscan    - Disco2 CSCAN (tela)"
	@echo ""
	@echo "ANÁLISE (PROJETO B):"
	@echo "  compare-disk-results - Gera relatório comparativo"
	@echo "  extract-disk-metrics - Extrai métricas para CSV"
	@echo ""
	@echo "UTILITÁRIOS:"
	@echo "  backup-disk         - Cria backup do disk.dat"
	@echo "  restore-disk        - Restaura disk.dat do backup"
	@echo "  clean               - Remove tudo (preserva disk.dat)"
	@echo "  clean-bin           - Remove apenas executáveis"
	@echo "  clean-output        - Remove apenas resultados"
	@echo "  rebuild             - Recompila tudo"
	@echo "  check-files         - Verifica arquivos necessários"
	@echo "  list-results        - Lista resultados disponíveis"
	@echo "  help                - Mostra esta ajuda"
	@echo ""
	@echo "PROJETO ATUAL: $(PROJECT)"
	@echo ""
	@echo "   IMPORTANTE: Testes do Projeto B modificam disk.dat!"
	@echo "   Faça backup antes de executar testes de disco."

# ============================================================================
# CONFIGURAÇÕES ESPECIAIS
# ============================================================================

# Declara alvos que não representam arquivos
.PHONY: all detect-project project-a project-b all-project-a all-project-b \
        clean clean-bin clean-output rebuild help \
        test-project-a test-project-b \
        test-disco1-all-schedulers test-disco2-all-schedulers \
        test-fcfs-only test-sstf-only test-cscan-only \
        disco1-all disco2-all scheduler-fcfs scheduler-sstf scheduler-cscan \
        disco1-fcfs disco1-sstf disco1-cscan \
        disco2-fcfs disco2-sstf disco2-cscan \
        disco1-all-tests disco2-all-tests \
        run-scheduler run-preempcao run-contab \
        run-disco1-fcfs run-disco1-sstf run-disco1-cscan \
        run-disco2-fcfs run-disco2-sstf run-disco2-cscan \
        compare-disk-results extract-disk-metrics \
        backup-disk restore-disk \
        check-files list-results show-project-status show-project-help

# Evita remoção de arquivos intermediários
.PRECIOUS: $(USER_OBJECTS)