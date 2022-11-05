# definizione del compilatore e dei flag di compilazione
# che vengono usate dalle regole implicite
CC=gcc
CFLAGS=-g -Wall -O -std=c99
LDLIBS=-lm -lrt -pthread

#Compilazione 
all: farm

#creazione eseguibile farm
farm: farm.o funzioni.o xerrori.o 
	$(CC) xerrori.o funzioni.o farm.o $(LDLIBS) -o farm

#Creazione eseguibile farm
farm.o: farm.c xerrori.h
	$(CC) $(CFLAGS) -c -o farm.o farm.c

#Creazione eseguibile xerrori
xerrori.o: xerrori.c xerrori.h
	$(CC) $(CFLAGS) -c -o xerrori.o xerrori.c

#Creazione eseguibile funzioni
funzioni.o: funzioni.c funzioni.h
	$(CC) $(CFLAGS) -c -o funzioni.o funzioni.c


# target che cancella eseguibili e file oggetto
clean:
	rm -f $(MAIN) *.o  

# target che crea l'archivio dei sorgenti
zip:
	zip $(MAIN).zip makefile *.c *.h *.py *.md
