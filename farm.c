#include "funzioni.h"

//Variabile utilizzata per far terminare il MasterWorker in caso arrivi il segnale SIGINT
volatile sig_atomic_t continua = 1;


//Funzione dell'handler
void gestore(int s) {
  continua = 0;   //interrompe il loop nel main
}




int main(int argc, char *argv[]){
  //controllo numero argomenti
  if(argc<2) {
      printf("Uso: %s file [file ...] -option \n",argv[0]);
      return 1;
  }

	
	//Parametri opzionali (inizializati di default)
	int nthread = 4;    //# thread worker da creare
	int qlen = 8;       //dimensione buffer prod-cons
	int delay = 0;      //ritado in millisecondi tra due
	                    //richieste del MasterWorker


// ---------   Gestione dei parametri in ingresso
	

	int inizio_param = 1;   //indica da dove iniziare a leggere i valori in argc[]
		                      //(considerando che getopt sposta tutte le stringe che sono parametri opzionali all'inizio)

	
	//Funzione che si occupa dell'aquisizione dei parametri opzionali da argv[]
	get_param_opt(argc, argv, &nthread, &qlen, &delay, &inizio_param);

	
	//fprintf(stdout,"\n\nFlags:\n -nthread: %d\n -qlen: %d\n -delay: %d\n",nthread, qlen, delay);
//_________________________________________________________

	
// ---------   GESTIONE DEI SEGNALI
	//definisce signal handler
  struct sigaction sa;

	//inizializza sa, copiando la gestione predefinita di SIGINT
  sigaction(SIGINT, NULL, &sa);
	
	//associa all'handler la funzione gestore
  sa.sa_handler = gestore;
	
	//associa a SIGINT l'handler
  sigaction(SIGINT, &sa, NULL);
//_________________________________________________________
	
	
// ---------   Inizializzazione dati MasterWorker e thread-worker
	char *buffer[qlen];         //buffer utilizzato da MasterWorker e thread-worker per scambiare i dati
	int cindex=0;               //indica la prima cella contenente dati da elaborare per i thread-worker 
	pthread_mutex_t read_buf = PTHREAD_MUTEX_INITIALIZER;        //mutex usata dai thread-worker per leggere il buffer
	sem_t sem_free_slots;                                        //semaforo che indica il numero di slot liberi per la scrittura
	sem_t sem_data_items;                                        //semaforo che indica il numero si slot contenente dati da elaborare
	xsem_init(&sem_free_slots,0,qlen,__LINE__,__FILE__);         //inizzializza il semafoto di slot liberi alla dimensione del buffer
  xsem_init(&sem_data_items,0,0,__LINE__,__FILE__);            //inizzializza il semafoto di items a 0

	//Creazione struct e settaggio delle variabili
	dati a;
	
	a.buffer = buffer;            //indirizzo buffer
	a.buf_size = qlen;            //dimensione del buffer
	a.cindex = &cindex;           //indirizzo variab. cindex
	a.rbuf = &read_buf;           //indirizzo variab. cmutex
	a.sem_data_items = &sem_data_items;   //semaforo items
	a.sem_free_slots = &sem_free_slots;   //semafori slots

	
	int pindex=0;              //indica la prima cella libera in scrittura per il MasterWorker
	pthread_t t[nthread];      //array per salvare il valore della thread_create
//_________________________________________________________

	
	//Creazione di nthread thread
	for(int i=0;i<nthread;i++) {    
		xpthread_create(&t[i],NULL,t_function,&a,__LINE__,__FILE__);   //crea un thread che esegue t_function passando la struttura a
  }


// ---------   MasterWorker 
	//Carica i nomi dei file passati sul buffer condiviso
	for(int i=inizio_param; i<argc; i++){
		xsem_wait(a.sem_free_slots,QUI);           //verifica che ci siano slot liberi nel buffer

		buffer[pindex % qlen] = argv[i];           //carica il nome del fine nel buffer
		pindex += 1;                               //incrementa l'indice del MasterWorker

		xsem_post(a.sem_data_items ,QUI);          //Indica che ci sono dati nel buffer da elaborare
		
		sleep((delay/1000));                       //aspetta [delay] millisecondi prima di caricare un'altro file sul buffer
		if(continua == 0){ break; }                //termina se gestore setta continua a 0  (se arriva un segnale SIGINT)
	}

	
	//Avvista tutti i thread che sono finiti i dati da elaborare utilizzando la stringa vuota ""
	for(int i=0; i<nthread; i++){
		xsem_wait(a.sem_free_slots,QUI);           //verifica che ci siano slot liberi nel buffer
		
		buffer[pindex % qlen] = "";                //carica stringa terminazione nel buffer
		pindex += 1;                               //incrementa l'indice del MasterWorker

		xsem_post(a.sem_data_items ,QUI);          //Indica che ci sono dati nel buffer da elaborare
	}

	
  //Aspetta che tutti i thread abbiano finito
  for(int i=0;i<nthread;i++) {
    xpthread_join(t[i],NULL,__LINE__,__FILE__);
  }

	
	//indica al server Collector di terminare
	send_to_coll("", 0, host, port, 1);

	
	//Distrugge mutex e semafori
	xpthread_mutex_destroy(&read_buf, QUI);
	xsem_destroy(&sem_data_items, QUI);
	xsem_destroy(&sem_free_slots, QUI);

	
	puts("\n\n\n ---  FINE  ----");
	return 0;
}