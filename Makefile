# ============================================================================
# Makefile para Testes do PingPongOS
# Compila e executa testes para diferentes funcionalidades do sistema
# ============================================================================

# Configurações do compilador
CC = gcc
CFLAGS = -Wall -std=c99
LDFLAGS = -lrt

# Flags opcionais (descomente conforme necessário)
# CFLAGS += -g          # Informações de debug para gdb
# CFLAGS += -O2         # Otimização
# CFLAGS += -Wextra     # Warnings extras

# Diretórios
SRC_DIR = .
BUILD_DIR = build
BIN_DIR = bin
OUTPUT_DIR = output

# Arquivos fonte que precisam ser compilados
USER_SOURCES = ppos-core-aux.c
USER_OBJECTS = $(USER_SOURCES:.c=.o)

# Objetos pré-compilados do sistema
SYSTEM_OBJECTS = queue.o ppos-all.o

# Todos os objetos necessários
ALL_OBJECTS = $(USER_OBJECTS) $(SYSTEM_OBJECTS)

# Arquivos de teste
TEST_SOURCES = pingpong-contab-prio.c \
               pingpong-dispatcher.c \
               pingpong-preempcao.c \
               pingpong-preempcao-stress.c \
               pingpong-scheduler.c

# Executáveis de teste (ficam na pasta bin/)
TEST_NAMES = pingpong-contab-prio \
             pingpong-dispatcher \
             pingpong-preempcao \
             pingpong-preempcao-stress \
             pingpong-scheduler

TEST_EXECUTABLES = $(addprefix $(BIN_DIR)/,$(TEST_NAMES))

# ============================================================================
# ALVOS PRINCIPAIS
# ============================================================================

# Compila todos os testes
all: $(BIN_DIR) $(TEST_EXECUTABLES)
	@echo "Todos os testes compilados com sucesso em $(BIN_DIR)/"

# Cria diretórios se não existirem
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(OUTPUT_DIR):
	@mkdir -p $(OUTPUT_DIR)

# ============================================================================
# COMPILAÇÃO DOS OBJETOS PRINCIPAIS
# ============================================================================

# Compila objetos do usuário
%.o: %.c
	@echo "Compilando objeto: $@"
	$(CC) $(CFLAGS) -c $< -o $@

# ============================================================================
# COMPILAÇÃO DOS TESTES INDIVIDUAIS
# ============================================================================

# Teste de contabilização e prioridades
$(BIN_DIR)/pingpong-contab-prio: pingpong-contab-prio.c $(ALL_OBJECTS) | $(BIN_DIR)
	@echo "Compilando teste de contabilização e prioridades..."
	$(CC) $(CFLAGS) -o $@ $< $(ALL_OBJECTS) $(LDFLAGS)

# Teste do dispatcher
$(BIN_DIR)/pingpong-dispatcher: pingpong-dispatcher.c $(ALL_OBJECTS) | $(BIN_DIR)
	@echo "Compilando teste do dispatcher..."
	$(CC) $(CFLAGS) -o $@ $< $(ALL_OBJECTS) $(LDFLAGS)

# Teste de preempção básica
$(BIN_DIR)/pingpong-preempcao: pingpong-preempcao.c $(ALL_OBJECTS) | $(BIN_DIR)
	@echo "Compilando teste de preempção..."
	$(CC) $(CFLAGS) -o $@ $< $(ALL_OBJECTS) $(LDFLAGS)

# Teste de stress de preempção
$(BIN_DIR)/pingpong-preempcao-stress: pingpong-preempcao-stress.c $(ALL_OBJECTS) | $(BIN_DIR)
	@echo "Compilando teste de stress de preempção..."
	$(CC) $(CFLAGS) -o $@ $< $(ALL_OBJECTS) $(LDFLAGS)

# Teste do escalonador
$(BIN_DIR)/pingpong-scheduler: pingpong-scheduler.c $(ALL_OBJECTS) | $(BIN_DIR)
	@echo "Compilando teste do escalonador..."
	$(CC) $(CFLAGS) -o $@ $< $(ALL_OBJECTS) $(LDFLAGS)

# ============================================================================
# ALIASES PARA COMPATIBILIDADE
# ============================================================================

# Aliases para testes individuais (para manter compatibilidade)
pingpong-contab-prio: $(BIN_DIR)/pingpong-contab-prio
pingpong-dispatcher: $(BIN_DIR)/pingpong-dispatcher
pingpong-preempcao: $(BIN_DIR)/pingpong-preempcao
pingpong-preempcao-stress: $(BIN_DIR)/pingpong-preempcao-stress
pingpong-scheduler: $(BIN_DIR)/pingpong-scheduler

# ============================================================================
# EXECUÇÃO DOS TESTES
# ============================================================================

# Executa todos os testes em sequência e salva resultados em arquivos
test-all: all $(OUTPUT_DIR)
	@echo ""
	@echo "Executando todos os testes e salvando resultados em $(OUTPUT_DIR)/..."
	@echo ""
	@echo "========================================="
	@echo "1. Teste do Dispatcher"
	@echo "========================================="
	@echo "Executando e salvando em $(OUTPUT_DIR)/pingpong-dispatcher.txt"
	-$(BIN_DIR)/pingpong-dispatcher > $(OUTPUT_DIR)/pingpong-dispatcher.txt 2>&1
	@echo "Concluído!"
	@echo ""
	@echo "========================================="
	@echo "2. Teste do Escalonador"
	@echo "========================================="
	@echo "Executando e salvando em $(OUTPUT_DIR)/pingpong-scheduler.txt"
	-$(BIN_DIR)/pingpong-scheduler > $(OUTPUT_DIR)/pingpong-scheduler.txt 2>&1
	@echo "Concluído!"
	@echo ""
	@echo "========================================="
	@echo "3. Teste de Preempção"
	@echo "========================================="
	@echo "Executando e salvando em $(OUTPUT_DIR)/pingpong-preempcao.txt"
	-$(BIN_DIR)/pingpong-preempcao > $(OUTPUT_DIR)/pingpong-preempcao.txt 2>&1
	@echo "Concluído!"
	@echo ""
	@echo "========================================="
	@echo "4. Teste de Contabilização e Prioridades"
	@echo "========================================="
	@echo "Executando e salvando em $(OUTPUT_DIR)/pingpong-contab-prio.txt"
	-$(BIN_DIR)/pingpong-contab-prio > $(OUTPUT_DIR)/pingpong-contab-prio.txt 2>&1
	@echo "Concluído!"
	@echo ""
	@echo "========================================="
	@echo "5. Teste de Stress de Preempção"
	@echo "========================================="
	@echo "Executando e salvando em $(OUTPUT_DIR)/pingpong-preempcao-stress.txt"
	-$(BIN_DIR)/pingpong-preempcao-stress > $(OUTPUT_DIR)/pingpong-preempcao-stress.txt 2>&1
	@echo "Concluído!"
	@echo ""
	@echo "Todos os testes executados! Resultados salvos em $(OUTPUT_DIR)/"

# Executa todos os testes mostrando na tela (sem salvar)
test-all-screen: all
	@echo ""
	@echo "Executando todos os testes (saída na tela)..."
	@echo ""
	@echo "========================================="
	@echo "1. Teste do Dispatcher"
	@echo "========================================="
	-$(BIN_DIR)/pingpong-dispatcher
	@echo ""
	@echo "========================================="
	@echo "2. Teste do Escalonador"
	@echo "========================================="
	-$(BIN_DIR)/pingpong-scheduler
	@echo ""
	@echo "========================================="
	@echo "3. Teste de Preempção"
	@echo "========================================="
	-$(BIN_DIR)/pingpong-preempcao
	@echo ""
	@echo "========================================="
	@echo "4. Teste de Contabilização e Prioridades"
	@echo "========================================="
	-$(BIN_DIR)/pingpong-contab-prio
	@echo ""
	@echo "========================================="
	@echo "5. Teste de Stress de Preempção"
	@echo "========================================="
	-$(BIN_DIR)/pingpong-preempcao-stress
	@echo ""
	@echo "Todos os testes executados!"

# Executa testes individuais salvando em arquivos
test-dispatcher: $(BIN_DIR)/pingpong-dispatcher $(OUTPUT_DIR)
	@echo "Executando teste do dispatcher e salvando em $(OUTPUT_DIR)/pingpong-dispatcher.txt"
	$(BIN_DIR)/pingpong-dispatcher > $(OUTPUT_DIR)/pingpong-dispatcher.txt 2>&1
	@echo "Resultado salvo em $(OUTPUT_DIR)/pingpong-dispatcher.txt"

test-scheduler: $(BIN_DIR)/pingpong-scheduler $(OUTPUT_DIR)
	@echo "Executando teste do escalonador e salvando em $(OUTPUT_DIR)/pingpong-scheduler.txt"
	$(BIN_DIR)/pingpong-scheduler > $(OUTPUT_DIR)/pingpong-scheduler.txt 2>&1
	@echo "Resultado salvo em $(OUTPUT_DIR)/pingpong-scheduler.txt"

test-preempcao: $(BIN_DIR)/pingpong-preempcao $(OUTPUT_DIR)
	@echo "Executando teste de preempção e salvando em $(OUTPUT_DIR)/pingpong-preempcao.txt"
	$(BIN_DIR)/pingpong-preempcao > $(OUTPUT_DIR)/pingpong-preempcao.txt 2>&1
	@echo "Resultado salvo em $(OUTPUT_DIR)/pingpong-preempcao.txt"

test-contab-prio: $(BIN_DIR)/pingpong-contab-prio $(OUTPUT_DIR)
	@echo "Executando teste de contabilização e salvando em $(OUTPUT_DIR)/pingpong-contab-prio.txt"
	$(BIN_DIR)/pingpong-contab-prio > $(OUTPUT_DIR)/pingpong-contab-prio.txt 2>&1
	@echo "Resultado salvo em $(OUTPUT_DIR)/pingpong-contab-prio.txt"

test-stress: $(BIN_DIR)/pingpong-preempcao-stress $(OUTPUT_DIR)
	@echo "Executando teste de stress e salvando em $(OUTPUT_DIR)/pingpong-preempcao-stress.txt"
	$(BIN_DIR)/pingpong-preempcao-stress > $(OUTPUT_DIR)/pingpong-preempcao-stress.txt 2>&1
	@echo "Resultado salvo em $(OUTPUT_DIR)/pingpong-preempcao-stress.txt"

# Executa testes individuais mostrando na tela
run-dispatcher: $(BIN_DIR)/pingpong-dispatcher
	@echo "Executando teste do dispatcher (saída na tela)..."
	$(BIN_DIR)/pingpong-dispatcher

run-scheduler: $(BIN_DIR)/pingpong-scheduler
	@echo "Executando teste do escalonador (saída na tela)..."
	$(BIN_DIR)/pingpong-scheduler

run-preempcao: $(BIN_DIR)/pingpong-preempcao
	@echo "Executando teste de preempção (saída na tela)..."
	$(BIN_DIR)/pingpong-preempcao

run-contab-prio: $(BIN_DIR)/pingpong-contab-prio
	@echo "Executando teste de contabilização (saída na tela)..."
	$(BIN_DIR)/pingpong-contab-prio

run-stress: $(BIN_DIR)/pingpong-preempcao-stress
	@echo "Executando teste de stress (saída na tela)..."
	$(BIN_DIR)/pingpong-preempcao-stress

# ============================================================================
# TESTES COM DEBUG
# ============================================================================

# Recompila com flags de debug ativadas
debug: clean
	@echo "Compilando com debug ativado..."
	$(MAKE) CFLAGS="$(CFLAGS) -DDEBUG -DDEBUG02" all

# Executa teste específico com debug
debug-test-%: debug
	@echo "Executando $* com debug..."
	$(BIN_DIR)/pingpong-$*

# ============================================================================
# UTILITÁRIOS
# ============================================================================

# Remove todos os arquivos compilados
clean:
	@echo "Limpando arquivos compilados..."
	rm -rf $(BIN_DIR) $(BUILD_DIR) $(OUTPUT_DIR)
	rm -f $(USER_OBJECTS)
	@echo "Limpeza concluída!"
	@echo "Mantidos: $(SYSTEM_OBJECTS) (objetos do sistema)"

# Remove apenas executáveis (mantém objetos)
clean-tests:
	@echo "Removendo executáveis de teste..."
	rm -rf $(BIN_DIR)

# Remove apenas resultados
clean-output:
	@echo "Removendo resultados dos testes..."
	rm -rf $(OUTPUT_DIR)

# Mostra os resultados salvos
show-output:
	@if [ -d "$(OUTPUT_DIR)" ]; then \
		echo "Resultados disponíveis em $(OUTPUT_DIR)/:"; \
		echo ""; \
		for file in $(OUTPUT_DIR)/*.txt; do \
			if [ -f "$file" ]; then \
				echo "----------------------------------------"; \
				echo "Arquivo: $file"; \
				echo "----------------------------------------"; \
				head -10 "$file"; \
				echo ""; \
			fi; \
		done; \
	else \
		echo "Nenhum resultado encontrado. Execute 'make test-all' primeiro."; \
	fi

# Lista os arquivos de resultado
list-output:
	@if [ -d "$(OUTPUT_DIR)" ]; then \
		echo "Arquivos de resultado em $(OUTPUT_DIR)/:"; \
		ls -la $(OUTPUT_DIR)/*.txt 2>/dev/null || echo "Nenhum arquivo .txt encontrado."; \
	else \
		echo "Pasta $(OUTPUT_DIR)/ não existe. Execute 'make test-all' primeiro."; \
	fi

# Força recompilação completa
rebuild: clean all

# Mostra informações sobre os alvos disponíveis
help:
	@echo "Alvos disponíveis:"
	@echo ""
	@echo "COMPILAÇÃO:"
	@echo "  all                 - Compila todos os testes em $(BIN_DIR)/"
	@echo "  pingpong-*          - Compila teste específico"
	@echo ""
	@echo "EXECUÇÃO (salva em arquivos):"
	@echo "  test-all            - Executa todos os testes e salva em $(OUTPUT_DIR)/"
	@echo "  test-dispatcher     - Executa teste do dispatcher e salva resultado"
	@echo "  test-scheduler      - Executa teste do escalonador e salva resultado"
	@echo "  test-preempcao      - Executa teste de preempção e salva resultado"
	@echo "  test-contab-prio    - Executa teste de contabilização e salva resultado"
	@echo "  test-stress         - Executa teste de stress e salva resultado"
	@echo ""
	@echo "EXECUÇÃO (mostra na tela):"
	@echo "  test-all-screen     - Executa todos os testes com saída na tela"
	@echo "  run-dispatcher      - Executa teste do dispatcher na tela"
	@echo "  run-scheduler       - Executa teste do escalonador na tela"
	@echo "  run-preempcao       - Executa teste de preempção na tela"
	@echo "  run-contab-prio     - Executa teste de contabilização na tela"
	@echo "  run-stress          - Executa teste de stress na tela"
	@echo ""
	@echo "DEBUG:"
	@echo "  debug               - Recompila com flags de debug"
	@echo "  debug-test-*        - Executa teste específico com debug"
	@echo ""
	@echo "UTILITÁRIOS:"
	@echo "  clean               - Remove todos os compilados e resultados"
	@echo "  clean-tests         - Remove apenas executáveis"
	@echo "  clean-output        - Remove apenas resultados"
	@echo "  show-output         - Mostra preview dos resultados salvos"
	@echo "  list-output         - Lista arquivos de resultado"
	@echo "  rebuild             - Força recompilação completa"
	@echo "  help                - Mostra esta ajuda"
	@echo ""
	@echo "ESTRUTURA:"
	@echo "  Executáveis: $(BIN_DIR)/"
	@echo "  Resultados: $(OUTPUT_DIR)/"
	@echo "  Objetos: raiz do projeto"

# ============================================================================
# VALIDAÇÃO
# ============================================================================

# Verifica se todos os arquivos necessários existem
check-sources:
	@echo "Verificando arquivos necessários..."
	@echo ""
	@echo "Arquivos fonte do usuário:"
	@for file in $(USER_SOURCES) $(TEST_SOURCES); do \
		if [ ! -f $$file ]; then \
			echo "ERRO: $$file"; \
		else \
			echo "OK: $$file"; \
		fi; \
	done
	@echo ""
	@echo "Objetos pré-compilados do sistema:"
	@for file in $(SYSTEM_OBJECTS); do \
		if [ ! -f $$file ]; then \
			echo "ERRO: $$file (necessário!)"; \
		else \
			echo "OK: $$file"; \
		fi; \
	done

# Descobre estrutura do projeto
discover:
	@echo "Descobrindo estrutura do projeto..."
	@echo "Arquivos .c encontrados:"
	@ls -1 *.c 2>/dev/null | grep -v pingpong- || echo "Nenhum arquivo .c encontrado"
	@echo ""
	@echo "Arquivos .h encontrados:"
	@ls -1 *.h 2>/dev/null || echo "Nenhum arquivo .h encontrado"
	@echo ""
	@echo "Arquivos .o encontrados:"
	@ls -1 *.o 2>/dev/null || echo "Nenhum arquivo .o encontrado"
	@echo ""
	@echo "Estrutura atual:"
	@echo "USER_SOURCES = $(USER_SOURCES)"
	@echo "SYSTEM_OBJECTS = $(SYSTEM_OBJECTS)"

# Mostra estatísticas dos testes
stats:
	@echo "Estatísticas do projeto:"
	@echo "Arquivos de teste: $(words $(TEST_SOURCES))"
	@echo "Arquivos fonte do usuário: $(words $(USER_SOURCES))"
	@echo "Objetos pré-compilados: $(words $(SYSTEM_OBJECTS))"
	@echo "Total de linhas nos fontes:"
	@wc -l $(TEST_SOURCES) $(USER_SOURCES) 2>/dev/null | tail -1 || echo "Erro ao contar linhas"

# ============================================================================
# CONFIGURAÇÕES ESPECIAIS
# ============================================================================

# Declara alvos que não representam arquivos
.PHONY: all clean clean-tests clean-output rebuild help test-all test-all-screen \
        test-dispatcher test-scheduler test-preempcao test-contab-prio test-stress \
        run-dispatcher run-scheduler run-preempcao run-contab-prio run-stress \
        show-output list-output debug check-sources stats discover \
        pingpong-contab-prio pingpong-dispatcher pingpong-preempcao \
        pingpong-preempcao-stress pingpong-scheduler

# Evita remoção de arquivos intermediários
.PRECIOUS: $(USER_OBJECTS)

# Configurações de dependências automáticas  
-include $(USER_OBJECTS:.o=.d)
