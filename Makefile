# Nome eseguibile
TARGET = shell

# Compilatore
CC = gcc
CFLAGS = -Wall -Wextra -std=c11

# Regola principale
all: $(TARGET)

# Compilazione diretta
$(TARGET): shell.c
	$(CC) $(CFLAGS) shell.c -o $(TARGET)

# Eseguire
run: $(TARGET)
	./$(TARGET)

# Pulizia
clean:
	rm -f $(TARGET)