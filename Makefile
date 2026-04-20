CC = gcc
CFLAGS = -Wall -Wextra -g

OBJ = shell.o fs.o aux.o

all: shell

shell: $(OBJ)
	$(CC) $(CFLAGS) -o shell $(OBJ)

shell.o: shell.c fs.h aux.h
	$(CC) $(CFLAGS) -c shell.c

fs.o: fs.c fs.h
	$(CC) $(CFLAGS) -c fs.c

aux.o: aux.c aux.h fs.h
	$(CC) $(CFLAGS) -c aux.c

clean:
	rm -f *.o shell
