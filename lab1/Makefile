CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CFLAGS  += -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE

BUILD   := build
SRC     := src

all: $(BUILD)/parent $(BUILD)/child1 $(BUILD)/child2

$(BUILD)/%: $(SRC)/%.c $(SRC)/common.h | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD):
	mkdir -p $@

run: all
	./$(BUILD)/parent

test: all
	./tests/run_tests.sh

clean:
	rm -rf $(BUILD)

.PHONY: all run test clean
