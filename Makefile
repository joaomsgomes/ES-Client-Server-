CC = gcc
CFLAGS = -Wall -Wextra -pthread -I./include -g
LDFLAGS = -pthread

# Diretórios
SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include

# Ficheiros fonte
SERVER_SRCS = $(SRC_DIR)/server/ES.c \
              $(SRC_DIR)/server/user_management.c \
              $(SRC_DIR)/server/udp_handlers.c \
              $(SRC_DIR)/common/protocol.c \
              $(SRC_DIR)/common/utils.c \
              $(SRC_DIR)/common/file_transfer.c

CLIENT_SRCS = $(SRC_DIR)/client/user.c \
              $(SRC_DIR)/common/protocol.c \
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
	$(CC) $(LDFLAGS) -o $@ $^

$(CLIENT): $(CLIENT_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(SERVER) $(CLIENT)
	rm -rf data/events/* data/users/*

run_server: $(SERVER)
	./$(SERVER) -p 58001 -v

run_client: $(CLIENT)
	./$(CLIENT) -n 127.0.0.1 -p 58001