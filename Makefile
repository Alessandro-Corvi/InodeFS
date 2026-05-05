# Nome eseguibile
TARGET = shell

# Compilatore
CC = gcc

# Flag
CFLAGS = -Wall -Wextra -g -DDEBUG
LDFLAGS = -lm

# Sorgenti e oggetti
SRCS = shell.c fs.c aux.c
OBJS = $(SRCS:.c=.o)

# Regola principale
all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Compilazione
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Pulizia
clean:
	rm -f $(OBJS) $(TARGET)

# Rebuild completo
re: clean all

.PHONY: all clean re