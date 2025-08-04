
CC := gcc
CFLAGS := -std=gnu11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Iinclude

SRCS := src/main.c src/ls.c src/cat.c src/cd.c src/ascii.c src/command.c src/simple_command.c src/exit.c src/pwd.c
PARSER_SRCS := src/shell.y src/shell.l
PARSER_OBJS := shell.tab.o lex.yy.o
OBJS := $(SRCS:.c=.o)
TARGET := cash

.PHONY: all clean 

all: $(TARGET)

# Link step (notice the -ll flag for the lexer) 
$(TARGET): $(OBJS) $(PARSER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -ll

# Compile step 
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

# Clean everything after the build
clean:
	rm -f $(OBJS) $(TARGET) $(PARSER_OBJS) shell.tab.c shell.tab.h lex.yy.c

