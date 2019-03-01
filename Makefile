CC=gcc
CFLAGS=--std=c99 --pedantic -Wall -W -Wmissing-prototypes 
LD=gcc
LDFLAGS=
EXEC=shell
MODULES=main.c shell.c
OBJECTS=main.o shell.o

all: $(EXEC)

shell: $(OBJECTS)
	$(LD) $(OBJECTS) $(GTK) -o $(EXEC)  $(LDFLAGS)

main.o: main.c
	$(CC) $(GTK) $(CFLAGS) -c main.c

shell.o: shell.c
	$(CC) $(GTK) $(CFLAGS) -c shell.c

clean:
	rm -f *.o   *~ shell