CC = cc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c11 -O2
CFLAGS += -I./src
LDFLAGS = -lcurl

BUILD_DIR = build

# Source files from all directories
DOMAIN_SRCS = src/domain/either.c src/domain/str.c src/domain/cli.c src/domain/template.c
LIBC_ADAPTER_SRCS = src/adapters/libc/cli.c src/adapters/libc/fs.c
CURL_ADAPTER_SRCS = src/adapters/curl/fetch.c
MAIN_SRCS = src/main.c

SRCS = $(MAIN_SRCS) $(DOMAIN_SRCS) $(LIBC_ADAPTER_SRCS) $(CURL_ADAPTER_SRCS)

# Object files (flatten into build/)
OBJS = $(BUILD_DIR)/main.o \
       $(BUILD_DIR)/domain_either.o \
       $(BUILD_DIR)/domain_str.o \
       $(BUILD_DIR)/domain_cli.o \
       $(BUILD_DIR)/domain_template.o \
       $(BUILD_DIR)/libc_cli.o \
       $(BUILD_DIR)/libc_fs.o \
       $(BUILD_DIR)/curl_fetch.o

TARGET = boil

.PHONY: all clean debug

all: $(BUILD_DIR) $(TARGET)

debug: CFLAGS += -g -DDEBUG -O0
debug: all

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Main
$(BUILD_DIR)/main.o: src/main.c
	$(CC) $(CFLAGS) -c $< -o $@

# Domain
$(BUILD_DIR)/domain_either.o: src/domain/either.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/domain_str.o: src/domain/str.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/domain_cli.o: src/domain/cli.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/domain_template.o: src/domain/template.c
	$(CC) $(CFLAGS) -c $< -o $@

# Adapters (libc - C standard library only)
$(BUILD_DIR)/libc_cli.o: src/adapters/libc/cli.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/libc_fs.o: src/adapters/libc/fs.c
	$(CC) $(CFLAGS) -c $< -o $@

# Adapters (curl - requires -lcurl)
$(BUILD_DIR)/curl_fetch.o: src/adapters/curl/fetch.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
