CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

# Dossiers
SRC_DIR = src
BIN_DIR = bin

# Fichiers exécutables cibles
TARGETS = $(BIN_DIR)/client $(BIN_DIR)/server

# Règle par défaut (lancée quand on tape juste "make")
all: $(TARGETS)

# Compilation de l'exécutable client
$(BIN_DIR)/client: $(SRC_DIR)/client.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@

# Compilation de l'exécutable server
$(BIN_DIR)/server: $(SRC_DIR)/server.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@

# Nettoyage des exécutables
clean:
	rm -rf $(BIN_DIR)

# Indique que "all" et "clean" ne sont pas des noms de fichiers
.PHONY: all clean