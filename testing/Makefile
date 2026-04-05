# Makefile для тестов обфускации MTProxy
# Copyright 2024-2026 MTProto Proxy Enhanced Project

CC = gcc
CFLAGS = -Wall -Wextra -g -I. -I../crypto -I../net
LDFLAGS = 

# Windows specific
ifeq ($(OS),Windows_NT)
    LDFLAGS += -lws2_32
else
    LDFLAGS += -lm
endif

# Тесты
TESTS = test_obfuscate test_padding test_fragmentation

# Исходные файлы
OBFUSCATE_SRC = ../crypto/obfuscate.c
PADDING_SRC = ../net/padding.c
FRAGMENTATION_SRC = ../net/fragmentation.c

.PHONY: all clean test

all: $(TESTS)

test_obfuscate: test_obfuscate.c $(OBFUSCATE_SRC)
	$(CC) $(CFLAGS) -o $@ test_obfuscate.c $(OBFUSCATE_SRC) $(LDFLAGS)

test_padding: test_padding.c $(PADDING_SRC)
	$(CC) $(CFLAGS) -o $@ test_padding.c $(PADDING_SRC) $(LDFLAGS)

test_fragmentation: test_fragmentation.c $(FRAGMENTATION_SRC)
	$(CC) $(CFLAGS) -o $@ test_fragmentation.c $(FRAGMENTATION_SRC) $(LDFLAGS)

test: $(TESTS)
	@echo ""
	@echo "=== Запуск тестов обфускации ==="
	@echo ""
	@echo "--- Тесты Obfuscate ---"
	./test_obfuscate
	@echo ""
	@echo "--- Тесты Padding ---"
	./test_padding
	@echo ""
	@echo "--- Тесты Fragmentation ---"
	./test_fragmentation
	@echo ""
	@echo "=== Все тесты завершены ==="

clean:
	rm -f $(TESTS) $(TESTS).exe
