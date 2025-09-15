# Makefile for YASH

CC      = gcc
CFLAGS  = -Wall -Wextra -g
LDFLAGS = -lreadline

# source files
SRCS    = main.c parser.c exec.c
OBJS    = $(SRCS:.c=.o)

# output binary
TARGET  = yash

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c parser.h exec.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)
