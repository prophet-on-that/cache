LIB-DIR := lib
BUILD-DIR := build
BIN-DIR := $(BUILD-DIR)/bin
TEST-DIR := test
LIB-SRC := $(shell find $(LIB-DIR) -type f -name '*.[c\|h]')
SRC-DIR := src
TEST-SRC := $(TEST-DIR)/test.c

all: $(BUILD-DIR)/test $(BIN-DIR)/server $(BIN-DIR)/client

$(BUILD-DIR)/test: $(LIB-SRC) $(TEST-SRC) | $(BUILD-DIR)
	gcc -g -W -o $@ $(filter %.c,$^)

.PHONY: test
test: $(BUILD-DIR)/test
	./$<

$(BUILD-DIR):
	mkdir -p $@

$(BIN-DIR): | $(BUILD-DIR)
	mkdir -p $@

$(BIN-DIR)/server: $(SRC-DIR)/server.c $(LIB-SRC) | $(BIN-DIR)
	gcc -g -W -Wformat -o $@ $(filter %.c,$^)

$(BIN-DIR)/client: $(SRC-DIR)/client.c $(LIB-SRC) | $(BIN-DIR)
	gcc -g -W -Wformat -o $@ $(filter %.c,$^)
