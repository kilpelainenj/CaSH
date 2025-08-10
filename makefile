CC := gcc
CFLAGS := -std=gnu11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Iinclude -g -Og

SRCS := src/main.c src/ls.c src/cat.c src/cd.c src/ascii.c src/command.c src/simple_command.c src/exit.c src/pwd.c src/export.c src/unset.c src/dirsum.c
PARSER_SRCS := src/shell.y src/shell.l
PARSER_OBJS := shell.tab.o lex.yy.o
OBJS := $(SRCS:.c=.o)
TARGET := cash

BREW_PREFIX := $(shell brew --prefix)

# cJSON via pkg-config
CFLAGS  += $(shell PKG_CONFIG_PATH="$(BREW_PREFIX)/lib/pkgconfig:$(PKG_CONFIG_PATH)" pkg-config --cflags libcjson)
LDLIBS  += $(shell PKG_CONFIG_PATH="$(BREW_PREFIX)/lib/pkgconfig:$(PKG_CONFIG_PATH)" pkg-config --libs   libcjson)

# libcurl via curl-config (keg-only in Homebrew)
CFLAGS  += $(shell "$(BREW_PREFIX)/opt/curl/bin/curl-config" --cflags)
LDLIBS  += $(shell "$(BREW_PREFIX)/opt/curl/bin/curl-config" --libs)

.PHONY: all clean
all: $(TARGET)

# Link step
$(TARGET): $(OBJS) $(PARSER_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ -ll $(LDLIBS)

# Compile
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Parser generation
shell.tab.c shell.tab.h: src/shell.y
	bison -d $<

lex.yy.c: src/shell.l shell.tab.h
	flex $<

shell.tab.o: shell.tab.c
	$(CC) $(CFLAGS) -c $< -o $@

lex.yy.o: lex.yy.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(PARSER_OBJS) shell.tab.c shell.tab.h lex.yy.c