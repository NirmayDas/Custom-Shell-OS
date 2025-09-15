CC      = gcc
CFLAGS  = -Wall -Wextra -g
LDFLAGS = -lreadline

# source files
SRCS    = main.c parser.c exec.c jobs.c
OBJS    = $(SRCS:.c=.o)

# output binary
TARGET  = yash

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c parser.h exec.h jobs.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)
