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
	rm -f es_server.db
	@echo "Database removed"

cleanall: clean cleandb
	rm -rf USERS/
	@echo "All data cleaned"

run_server: $(SERVER)
	./$(SERVER)

run_client: $(CLIENT)
	./$(CLIENT) -n 127.0.0.1 -p 58000

# Database queries
listusers:
	@echo "=== Users in Database ==="
	@sqlite3 es_server.db "SELECT uid, created_at FROM users;" 2>/dev/null || echo "No database found or no users"

listevents:
	@echo "=== Events in Database ==="
	@sqlite3 es_server.db "SELECT eid, uid, name, date, total_seats, reserved_seats, filename, file_size, state FROM events ORDER BY eid;" 2>/dev/null || echo "No database found or no events"

eventdetails:
	@echo "=== Event Details (with formatting) ==="
	@sqlite3 es_server.db -column -header "SELECT eid AS 'EID', name AS 'Name', date AS 'Date', total_seats AS 'Seats', reserved_seats AS 'Reserved', filename AS 'File', file_size AS 'Size(bytes)', CASE state WHEN 1 THEN 'ACTIVE' WHEN 0 THEN 'PAST' WHEN 2 THEN 'SOLD_OUT' WHEN 3 THEN 'CLOSED' END AS 'State' FROM events ORDER BY eid;" 2>/dev/null || echo "No database found"

checkdb:
	@echo "=== Database Statistics ==="
	@sqlite3 es_server.db "SELECT 'Users: ' || COUNT(*) FROM users; SELECT 'Events: ' || COUNT(*) FROM events;" 2>/dev/null || echo "No database found"

.PHONY: cleandb cleanall listusers listevents eventdetails checkdb run_server run_client