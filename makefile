CC := gcc
CFLAGS := -std=gnu11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Iinclude -g -Og

SRCS := src/main.c src/ls.c src/cat.c src/cd.c src/ascii.c src/command.c src/simple_command.c src/exit.c src/pwd.c src/export.c src/unset.c src/dirsum.c src/builtins.c
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

FLEX_PREFIX := $(shell brew --prefix flex 2>/dev/null)
ifeq ($(FLEX_PREFIX),)
  FLEX_LIBS := -ll
else
  FLEX_LIBS := -L$(FLEX_PREFIX)/lib -lfl
endif

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJS) $(PARSER_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(FLEX_LIBS) $(LDLIBS)

# Compile C
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

#   Catch2 v3 testing
CXX := g++
CXXFLAGS := -std=c++17 -g -O0 -Iinclude
CATCH2_PREFIX := $(shell brew --prefix catch2 2>/dev/null)
CXXFLAGS += -I$(CATCH2_PREFIX)/include
TEST_LIBS := -L$(CATCH2_PREFIX)/lib -lCatch2Main -lCatch2  

CORE_SRCS := $(filter-out src/main.c,$(SRCS))
CORE_OBJS := $(CORE_SRCS:.c=.o)
TEST_SRCS := $(wildcard tests/*.cpp \
						tests/unit/*.cpp \
						tests/integration/*.cpp)

TEST_BIN := test_runner

.PHONY: tests
tests: $(TEST_BIN)

$(TEST_BIN): $(CORE_OBJS) $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_SRCS) $(CORE_OBJS) $(LDLIBS) $(TEST_LIBS)

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET) $(PARSER_OBJS) shell.tab.c shell.tab.h lex.yy.c $(TEST_BIN)