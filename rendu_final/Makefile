CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

SRC_DIR = src
BIN_DIR = bin

TARGETS = $(BIN_DIR)/client $(BIN_DIR)/server

all: $(TARGETS)

COMMON = $(SRC_DIR)/common.c
CLIENT_SRC = $(wildcard $(SRC_DIR)/client/*.c)
SERVER_SRC = $(wildcard $(SRC_DIR)/server/*.c)
HEADERS = $(wildcard include/*.h)

$(BIN_DIR)/client: $(CLIENT_SRC) $(COMMON) $(HEADERS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(filter %.c,$^) -o $@

$(BIN_DIR)/server: $(SERVER_SRC) $(COMMON) $(HEADERS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(filter %.c,$^) -o $@

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean
