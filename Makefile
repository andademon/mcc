CC=gcc
CFLAGS=-O0 -g
SRCS=main.c scanner.c parser.c util.c codegen.c sema.c
OBJS=$(SRCS:.c=.o)
TARGET=main

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
