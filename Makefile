# Diretórios
SRC_DIR = src
BIN_DIR = bin

# Compilador e flags
CC = gcc
CFLAGS = -Wall -g

# Alvo padrão
all: $(BIN_DIR)/client $(BIN_DIR)/server

# Binário do cliente
$(BIN_DIR)/client: $(SRC_DIR)/client.c $(BIN_DIR)/common.o
	$(CC) $(CFLAGS) $^ -o $@

# Binário do servidor
$(BIN_DIR)/server: $(SRC_DIR)/server.c $(BIN_DIR)/common.o
	$(CC) $(CFLAGS) $^ -o $@

# Objeto comum
$(BIN_DIR)/common.o: $(SRC_DIR)/common.c
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Limpeza
clean:
	rm -rf $(BIN_DIR)

# PHONY para evitar conflitos com arquivos reais
.PHONY: all clean
