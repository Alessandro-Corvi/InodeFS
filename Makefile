CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lm

OBJS = shell.o fs.o aux.o

all: shell

shell: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

shell.o: shell.c fs.h aux.h
	$(CC) $(CFLAGS) -c $< -o $@

fs.o: fs.c fs.h aux.h
	$(CC) $(CFLAGS) -c $< -o $@

aux.o: aux.c aux.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) shell

.PHONY: all clean