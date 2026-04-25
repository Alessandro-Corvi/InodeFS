# Nome del compilatore
CC = gcc

# Flag di compilazione
CFLAGS = -Wall -Wextra -std=c11

# Nome dell'eseguibile
TARGET = shell

# File sorgente
SRC = shell.c

# File oggetto
OBJ = $(SRC:.c=.o)

# Regola principale
all: $(TARGET)

# Link finale
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Compilazione dei .c in .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Pulizia
clean:
	rm -f $(OBJ) $(TARGET)