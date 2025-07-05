
CC := gcc
CFLAGS := -std=gnu11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra

SRCS := main.c ls.c
OBJS := $(SRCS:.c=.o)
TARGET := cash

.PHONY: all clean 

all: $(TARGET)

# Link step 
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile step 
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean everything after the build
clean:
	rm -f $(OBJS) $(TARGET)

