LIB-DIR := lib
BUILD-DIR := build
TEST-DIR := test
LIB-SRC := $(shell find $(LIB-DIR) -type f -name '*.[c\|h]')
TEST-SRC := $(TEST-DIR)/test.c

$(BUILD-DIR)/test: $(LIB-SRC) $(TEST-SRC)
	gcc -g -W -o $@ $(filter %.c,$^)

.PHONY: test
test: $(BUILD-DIR)/test
	./$<
