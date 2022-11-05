/*
  Descrizione:
    Contiene tutte le funzioni ausiliare utilizzate nel programma farm.c,
    i prototipi sono in funzioni.h
*/

#include "xerrori.h"
#define QUI __LINE__,__FILE__
#define host "127.0.0.1"
#define port 57451

//struct contenente i parametri da passare ai thread 
typedef struct{
	char **buffer;              //buffer
	int buf_size;               //dimensione del buffer
  int *cindex;                //indice nel buffer thread-worker
	pthread_mutex_t *rbuf;      //mutex per leggere dal file
  pthread_mutex_t *wbuf;      //mutex per scrivere nel buffer
  sem_t *sem_free_slots;      //semaforo che indica il numero di slots liberi
  sem_t *sem_data_items;      //semaforo che indica il numero di elemeti presenti
} dati; 


//Si occupa di gestire i parametri opzionali in input al programma farm
void get_param_opt(int argc, char *argv[], int *nt, int *ql, int *de, int *i_p);

//Corpo dei thread-worker
void *t_function(void *arg);

//Calcola la somma dei long contenuti nel file
long read_file(FILE *f);

//Scrive nel buffer i dati
ssize_t writen(int fd, void *ptr, size_t n);

//Invia al server Collector: nome_file, valore
void send_to_coll(char* nome_file, long valore, char* HOST, int PORT, int termina);