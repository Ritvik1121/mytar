C = gcc
CFLAGS = -Wall -pedantic -g

all: mytar

mytar:mytar.c table.c extract.c shared.c create.c
	$(CC) $(CFLAGS) -o mytar mytar.c table.c extract.c create.c shared.c -lm

clean:
	rm -f *.o
