# Compiler and flags
CC = gcc

# Original Strict flags
CFLAGS_BASE = -std=c99 \
         -pedantic \
         -pedantic-errors \
         -Wall \
         -Wextra \
         -Wformat=2 \
         -Wformat-security \
         -Wnull-dereference \
         -Wstack-protector \
         -Wtrampolines \
         -Walloca \
         -Wvla \
         -Warray-bounds=2 \
         -Wimplicit-fallthrough=3 \
         -Wshift-overflow=2 \
         -Wcast-qual \
         -Wcast-align=strict \
         -Wconversion \
         -Wsign-conversion \
         -Wlogical-op \
         -Wduplicated-cond \
         -Wduplicated-branches \
         -Wrestrict \
         -Wnested-externs \
         -Winline \
         -Wundef \
         -Wstrict-prototypes \
         -Wmissing-prototypes \
         -Wmissing-declarations \
         -Wredundant-decls \
         -Wshadow \
         -Wwrite-strings \
         -Wfloat-equal \
         -Wpointer-arith \
         -Wbad-function-cast \
         -Wold-style-definition

# Strict flags for project source
CFLAGS = $(CFLAGS_BASE) -Isrc -Isrc/include

# Suppress noise-prone warnings for external library code
CFLAGS_LIB = $(CFLAGS_BASE) -Wno-conversion -Wno-sign-conversion -Wno-cast-qual -Isrc -Isrc/include

HARDENING = -D_FORTIFY_SOURCE=2 \
            -fstack-protector-strong \
            -fPIE \
            -fstack-clash-protection \
            -fcf-protection

LDFLAGS = -Wl,-z,relro \
          -Wl,-z,now \
          -Wl,-z,noexecstack \
          -Wl,-z,separate-code \
          -pie \
          -flto

OPTFLAGS = -O3 -march=native -flto

GTK_FLAGS = `pkg-config --cflags --libs gtk+-3.0`

BIN_DIR = bin
OBJ_DIR = bin/obj
SRC_DIR = src
ASSETS_DIR = assets
TARGET = $(BIN_DIR)/timer
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/include/set.c $(SRC_DIR)/include/stringlib.c
OBJS = $(OBJ_DIR)/main.o $(OBJ_DIR)/set.o $(OBJ_DIR)/stringlib.o $(OBJ_DIR)/resources.o
RESOURCES_SRC = $(OBJ_DIR)/resources.c

.PHONY: all directories format lint install uninstall clean

all: clean format lint directories $(TARGET)

directories:
	mkdir -p $(BIN_DIR) $(OBJ_DIR)

$(TARGET): $(OBJS) | directories
	$(CC) $(CFLAGS) $(HARDENING) $(OPTFLAGS) $(LDFLAGS) $(OBJS) $(GTK_FLAGS) -o $@

$(OBJ_DIR)/resources.o: $(RESOURCES_SRC) | directories
	$(CC) $(CFLAGS) $(HARDENING) $(OPTFLAGS) $(GTK_FLAGS) -c $< -o $@

$(RESOURCES_SRC): $(ASSETS_DIR)/timer.gresource.xml $(ASSETS_DIR)/style.css | directories
	cd $(ASSETS_DIR) && glib-compile-resources timer.gresource.xml --target=../$(RESOURCES_SRC) --generate-source

$(OBJ_DIR)/main.o: $(SRC_DIR)/main.c | directories
	$(CC) $(CFLAGS) $(HARDENING) $(OPTFLAGS) $(GTK_FLAGS) -c $< -o $@

$(OBJ_DIR)/set.o: $(SRC_DIR)/include/set.c | directories
	$(CC) $(CFLAGS_LIB) $(HARDENING) $(OPTFLAGS) $(GTK_FLAGS) -c $< -o $@

$(OBJ_DIR)/stringlib.o: $(SRC_DIR)/include/stringlib.c | directories
	$(CC) $(CFLAGS_LIB) $(HARDENING) $(OPTFLAGS) $(GTK_FLAGS) -c $< -o $@

format:
	clang-format -style=file:./.clang-format -i $(SRCS)
	mbake format --config ./.bake.toml Makefile

CLANG_TIDY_CHECKS = -checks=-bugprone-easily-swappable-parameters,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling
CLANG_TIDY_FLAGS = -std=c99 -pedantic -Wall -Wextra -Isrc

lint:
	clang-tidy $(CLANG_TIDY_CHECKS) $(SRC_DIR)/main.c -- $(shell pkg-config --cflags gtk+-3.0 | sed 's/-I/-isystem /g') $(CLANG_TIDY_FLAGS)
	mbake validate --config ./.bake.toml Makefile

test: tests/test_stringlib.c tests/test_timer.c $(SRC_DIR)/include/stringlib.c | directories
	$(CC) -Isrc/include -Itests/include tests/test_stringlib.c $(SRC_DIR)/include/stringlib.c -o bin/runner_string
	./bin/runner_string
	$(CC) -Isrc/include -Itests/include tests/test_timer.c -o bin/runner_timer
	./bin/runner_timer

install: $(TARGET)
	mkdir -p $(HOME)/.local/bin
	install -m 755 $(TARGET) $(HOME)/.local/bin/timer

uninstall:
	rm -f $(HOME)/.local/bin/timer

clean:
	rm -rf $(BIN_DIR)
