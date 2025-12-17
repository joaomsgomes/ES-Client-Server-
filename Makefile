CC = gcc
CFLAGS = -Wall -Wextra -pthread -I./include -g
LDFLAGS = -pthread

# Diretórios
SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include

# Ficheiros fonte - USANDO sistema de ficheiros
SERVER_SRCS = $(SRC_DIR)/server/ES.c \
              $(SRC_DIR)/server/file_system.c \
              $(SRC_DIR)/server/user_database.c \
              $(SRC_DIR)/server/event_database.c \
              $(SRC_DIR)/server/tcp_handlers.c \
              $(SRC_DIR)/server/udp_handlers.c \
              $(SRC_DIR)/common/utils.c \
              $(SRC_DIR)/common/file_transfer.c

CLIENT_SRCS = $(SRC_DIR)/client/user.c \
              $(SRC_DIR)/common/utils.c

# Ficheiros objeto
SERVER_OBJS = $(SERVER_SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
CLIENT_OBJS = $(CLIENT_SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Executáveis
SERVER = ES
CLIENT = user

.PHONY: all clean dirs

all: dirs $(SERVER) $(CLIENT)

dirs:
	@mkdir -p $(BUILD_DIR)/server
	@mkdir -p $(BUILD_DIR)/client
	@mkdir -p $(BUILD_DIR)/common

$(SERVER): $(SERVER_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
	@echo "✓ ES Server compiled (file-based)"

$(CLIENT): $(CLIENT_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(SERVER) $(CLIENT)
	@echo "Clean complete"

cleandb:
	@echo "🗑️  Limpando base de dados (file-based)..."
	@rm -rf USERS/* EVENTS/* 2>/dev/null || true
	@mkdir -p USERS EVENTS
	@echo "✅ Base de dados limpa (USERS/ e EVENTS/ vazios)"

cleanall: clean cleandb
	@echo "✅ Limpeza completa: binários + base de dados"

run_server: $(SERVER)
	./$(SERVER)

run_client: $(CLIENT)
	./$(CLIENT) -n 127.0.0.1 -p 58000

# Database queries (file-based system)
listusers:
	@echo "=== Users in Database ==="
	@if [ -d "USERS" ]; then \
		for user_dir in USERS/*/; do \
			if [ -d "$$user_dir" ]; then \
				uid=$$(basename "$$user_dir"); \
				echo "UID: $$uid"; \
			fi \
		done \
	else \
		echo "No USERS directory found"; \
	fi

listevents:
	@echo "=== Events in Database ==="
	@if [ -d "EVENTS" ]; then \
		for event_dir in EVENTS/*/; do \
			if [ -d "$$event_dir" ]; then \
				eid=$$(basename "$$event_dir"); \
				start_file="$$event_dir/START_$$eid.txt"; \
				if [ -f "$$start_file" ]; then \
					echo "EID $$eid: $$(cat $$start_file)"; \
				fi \
			fi \
		done \
	else \
		echo "No EVENTS directory found"; \
	fi

checkdb:
	@echo "=== Database Statistics ==="
	@echo -n "Users: "; ls -1d USERS/*/ 2>/dev/null | wc -l || echo "0"
	@echo -n "Events: "; ls -1d EVENTS/*/ 2>/dev/null | wc -l || echo "0"

.PHONY: cleandb cleanall listusers listevents checkdb run_server run_client

# =========================
# Remote test runner (tejo)
# =========================

# Configurações (podes passar por CLI: make test_all TARGET_IP=... TARGET_PORT=...)
TEJO_HOST ?= tejo.tecnico.ulisboa.pt
TEJO_PORT ?= 59000
TARGET_IP ?= 127.0.0.1
TARGET_PORT ?= 58000

# Sequência típica (ajusta se quiseres incluir mais scripts)
TEST_SCRIPTS ?= 1 2 3 4 5 6 7 8 9 10 11 12 21 22 23 24 25 26 27 28 29

# Onde guardar relatórios
TEST_DIR ?= tests
REPORT ?= report.html

.PHONY: test_all test_one test_1to8 test_9to12 test_21to29 open_report

# Testa scripts 1-8 (funcionalidades básicas UDP/TCP)
# Requer DB limpa no início
test_1to8:
	@echo "===========================================" ;
	@echo "  Scripts 1-8: Funcionalidades Básicas    " ;
	@echo "  Limpando base de dados...               " ;
	@echo "===========================================" ;
	@$(MAKE) cleandb ;
	@sleep 1 ;
	@set -u; \
	mkdir -p "$(TEST_DIR)"; \
	echo "▶ Executando scripts 1 a 8..."; \
	for s in 1 2 3 4 5 6 7 8; do \
		echo "▶ Running script $$s ..."; \
		if printf "$(TARGET_IP) $(TARGET_PORT) $$s\n" | nc "$(TEJO_HOST)" "$(TEJO_PORT)" > "$(REPORT)" 2>&1; then \
			cp -f "$(REPORT)" "$(TEST_DIR)/report_$$s.html" 2>/dev/null || true; \
			if grep -Eqi '(FAIL|ERROR)' "$(REPORT)"; then \
				echo "✗ FAIL on script $$s"; \
			else \
				echo "✓ PASS script $$s"; \
			fi; \
		else \
			echo "✗ ERROR: nc failed on script $$s"; \
		fi; \
		echo ""; \
	done; \
	echo "✅ Scripts 1-8 COMPLETOS"

# Testa scripts 9-12 (gestão e consistência da DB)
# Requer DB limpa no início (após scripts 1-8)
test_9to12:
	@echo "===========================================" ;
	@echo "  Scripts 9-12: Gestão e Consistência     " ;
	@echo "  Limpando base de dados...               " ;
	@echo "===========================================" ;
	@$(MAKE) cleandb ;
	@sleep 1 ;
	@set -u; \
	mkdir -p "$(TEST_DIR)"; \
	echo "▶ Executando scripts 9 a 12..."; \
	for s in 9 10 11 12; do \
		echo "▶ Running script $$s ..."; \
		if printf "$(TARGET_IP) $(TARGET_PORT) $$s\n" | nc "$(TEJO_HOST)" "$(TEJO_PORT)" > "$(REPORT)" 2>&1; then \
			cp -f "$(REPORT)" "$(TEST_DIR)/report_$$s.html" 2>/dev/null || true; \
			if grep -Eqi '(FAIL|ERROR)' "$(REPORT)"; then \
				echo "✗ FAIL on script $$s"; \
			else \
				echo "✓ PASS script $$s"; \
			fi; \
		else \
			echo "✗ ERROR: nc failed on script $$s"; \
		fi; \
		echo ""; \
	done; \
	echo "✅ Scripts 9-12 COMPLETOS"

# Testa scripts 21-29 (testes adicionais)
test_21to29:
	@echo "===========================================" ;
	@echo "  Scripts 21-29: Testes Adicionais        " ;
	@echo "===========================================" ;
	@set -u; \
	mkdir -p "$(TEST_DIR)"; \
	echo "▶ Executando scripts 21 a 29..."; \
	for s in 21 22 23 24 25 26 27 28 29; do \
		echo "▶ Running script $$s ..."; \
		if printf "$(TARGET_IP) $(TARGET_PORT) $$s\n" | nc "$(TEJO_HOST)" "$(TEJO_PORT)" > "$(REPORT)" 2>&1; then \
			cp -f "$(REPORT)" "$(TEST_DIR)/report_$$s.html" 2>/dev/null || true; \
			if grep -Eqi '(FAIL|ERROR)' "$(REPORT)"; then \
				echo "✗ FAIL on script $$s"; \
			else \
				echo "✓ PASS script $$s"; \
			fi; \
		else \
			echo "✗ ERROR: nc failed on script $$s"; \
		fi; \
		echo ""; \
	done; \
	echo "✅ Scripts 21-29 COMPLETOS"

# Executa TODAS as sequências na ordem correta com limpezas automáticas
test_all:
	@echo "=========================================" ;
	@echo "  SEQUÊNCIA COMPLETA DE TESTES           " ;
	@echo "  Target: $(TARGET_IP):$(TARGET_PORT)    " ;
	@echo "  Tejo: $(TEJO_HOST):$(TEJO_PORT)        " ;
	@echo "=========================================" ;
	@echo "" ;
	@$(MAKE) test_1to8 TARGET_IP=$(TARGET_IP) TARGET_PORT=$(TARGET_PORT) ;
	@echo "" ;
	@$(MAKE) test_9to12 TARGET_IP=$(TARGET_IP) TARGET_PORT=$(TARGET_PORT) ;
	@echo "" ;
	@$(MAKE) test_21to29 TARGET_IP=$(TARGET_IP) TARGET_PORT=$(TARGET_PORT) ;
	@echo "" ;
	@echo "=========================================" ;
	@echo "✅ TODOS OS TESTES COMPLETADOS           " ;
	@echo "  Relatórios em: $(TEST_DIR)/report_*.html" ;
	@echo "========================================="

# Corre só 1 script: make test_one S=06
test_one:
	@set -eu; \
	: "$${S:?Usage: make test_one S=06}"; \
	mkdir -p "$(TEST_DIR)"; \
	echo "▶ Running script $$S ..."; \
	printf "$(TARGET_IP) $(TARGET_PORT) $$S\n" | nc "$(TEJO_HOST)" "$(TEJO_PORT)" > "$(REPORT)"; \
	cp -f "$(REPORT)" "$(TEST_DIR)/report_$$S.html"; \
	echo "Saved: $(TEST_DIR)/report_$$S.html (and $(REPORT))"

open_report:
	@set -eu; \
	if command -v xdg-open >/dev/null 2>&1; then xdg-open "$(REPORT)"; \
	elif command -v open >/dev/null 2>&1; then open "$(REPORT)"; \
	else echo "Open manually: $(REPORT)"; fi
