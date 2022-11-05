/*
  Descrizione:
    Contiene il prototipo  delle funzioni ausiliare utilizzate nel programma farm.c,
    il copro delle funzioni sono in funzioni.c
*/


#include "funzioni.h"
#include "xerrori.h"


/* ----- get_param_opt
 *  Input:
 *   - argc        dimensione *argv[]
 *   - *argv[]     array di stringhe contenente i parametri passati al main del programma farm
 *   - *nt         puntatore alla variabile nthread nel main
 *   - *ql         puntatore alla variabile qlen nel main
 *   - *de         puntatore alla variabile delay nel main
 *   - *i_p        puntatore alla variabile inizio_parametri nel main
 *
 *  Output:
 *   - void
 *
 *  Descrizione:
 *   Estrae gli argomenti opzionali passati come parametri e setta le variabili nel main
 *   (utilizzo funzione getopt per prendere i parametri opzionali)
 *
*/
void get_param_opt(int argc, char *argv[], int *nt, int *ql, int *de, int *i_p){

	int opt;   //nome del segnale opzionali passato
	//optarg:  variabile definita dal sistema che contiene valore del parametro opzionale
	
	//Controlla tutti i parametri passati in cerca dei parametri opzionali indicati
	while((opt = getopt(argc, argv, "n:q:t:")) != -1){
		//Verifica di quale parametro opzionale si tratta
		switch(opt)
		{
			case 'n':
				if(optarg != NULL && atoi(optarg)>0){      //controlla che il valore passato sia valido
					*nt = atoi(optarg);                      //setta nthread nel main al valore ottenuto
					*i_p += 2;                               //incremento inizio_parametri nel main
				}
				else{    //termina il programma se viene passato un valore non valido
					xtermina("\n!-- Numero thread > 0 --!\n",QUI);
				}
		
			break;
			
			case 'q':		              
				if(optarg != NULL && atoi(optarg)>0){      //controlla che il valore passato sia valido
					*ql = atoi(optarg);                      //setta qlen nel main al valore ottenuto
					*i_p += 2;                               //incremento inizio_parametri nel main
				}
				else{    //termina il programma se viene passato un valore non valido
					xtermina("\n!-- Dimensione buffer > 0 --!\n",QUI);
				}
			break;
			
			case 't':
				if(optarg != NULL && atoi(optarg)>=0){      //controlla che il valore passato sia valido
					*de = atoi(optarg);                       //setta delay nel main al valore ottenuto
					*i_p += 2;                                //incremento inizio_parametri nel main
				}
				else{    //termina il programma se viene passato un valore non valido
					xtermina("\n!-- Delay > 0 --!\n",QUI);
				}
			break;

			default:
				*i_p += 2;                                //incremento inizio_parametri nel main
				xtermina("\n!-- Parametro Opzionale non riconosciuto --!\n",QUI);
			break;
		}
	}
}



/* ----- read_file
 *  Input:
 *   - *nome_file    stringa che indentifica il nome del file da leggere
 *
 *  Output:
 *   - long          risultato della sommatoria dei valori in nome_file
 *
 *  Descrizione:
 *   apre il file nome_file, legge i long contenuti in esso e calcola il risultato
 *   attraverso una formula
*/
long read_file(FILE *f){

	long num = 0;       //dove viene salvato il numero letto dal file
	long somma = 0;     //mantiene il risultato dei valori letti dal file
	int e=0;            //usata per salvare gli output delle funzioni
	int i=0;            //conta quanti numeri vengono letti dal file

	//legge numeri da file finché ci sono
	while(true){
		e = fread(&num,sizeof(num), 1, f);    //legge un numero
		if(e!=1){ break; }                    //esce se la lettura non è andata a buon fine
		
		somma += (i*num);                     //calcola il risultato come (numero_letto * posizione_numero)
		i++;                                  //incrementa il # di long letti
	}
	
	return somma;
}



/* ----- t_function
 *  Input:
 *   - *arg          strutta di tipo 'dati' che contiene le variabili condivise utili al thread
 *
 *  Output:
 *   - void
 *
 *  Descrizione:
 *   Funzione dei thread-worker che prendono i nomi dei file dal buffer, elaborano le informazioni e
 *   inviano al server collector la coppia (nome file, valore)
*/
void *t_function(void *arg){
	
	dati *a = (dati *) arg;      //casting a struct di tipo 'dati'

	long somma=0;                //contiene il risultato della sommatoria calcolata su un file
	char *nome_file;             //contiene il nome del file da eleborare

	
	//elabora dati finchè con rivece il segnali di interruzione 
	while(true) {
		xsem_wait(a->sem_data_items,QUI);        //attende sul semaforo items che ci siano dati da elaborare
		xpthread_mutex_lock(a->rbuf ,QUI);       //aquisisce la mutex per evitare conflitti con altri thread

		nome_file = a->buffer[*(a->cindex) % a->buf_size];     //prendo il nome_file dal buffer
		*(a->cindex) += 1;                                     //incrementa indice condiviso dei thread_worker

    xsem_post(a->sem_free_slots,QUI);        //incrementa il semaforo slot per indicare che ci sono slot liberi nel buffer
		xpthread_mutex_unlock(a->rbuf,QUI);      //rilascia la mutex

		if(strcmp(nome_file,"")==0){  break;  }  //termina se legge dal buffer la stringa ""

		FILE *f = fopen(nome_file, "rb");       //apre il file binario nome_file in lettuta
		if(f==NULL) {
			fprintf(stderr,"\n\n  -> Errore aprtura file:  %s\n",nome_file);
			continue;
		}
		somma = read_file(f);                //chiama la funzione read_file che elabora i dati nel file e restituisce il risultato
		xfclose(f, QUI);	                                 //Chiude il descrittore di file aperto
		send_to_coll(nome_file, somma, host, port, 0);     //invia (nome del file, risultato) al server collector
  }

	pthread_exit(NULL);    //termina restituiendo NULL
}


/* ----- writen
 *  Input:
 *   - fd         file descriptor in cui si vuole scrivere  (in questo caso il socket)
 *   - *ptr       zona di memoria dove ci sono i dati da inviare
 *   - n          numero byte da scrivere
 *
 *
 *  Output:
 *   - ssize_t    numero byte scritti,  -1 se non è riuscito a scrivere nessun byte
 *
 *  Descrizione:
 *   funzione che implementa la write, scrivendo esattamente n byte sul file descriptor fd
*/
ssize_t writen(int fd, void *ptr, size_t n) {  
   size_t   nleft;          //numero byte rimanenti da scrivere
	 nleft = n;               //inizialmente sono n
   ssize_t  nwritten;       //numero byte scritti
 
   //scrive fino a quando ci sono ancora byte da scrivere
   while (nleft > 0) {
    if((nwritten = write(fd, ptr, nleft)) < 0) {       //controllo che non ci siano errori nella write
      if(nleft == n){ return -1; }                     //ritorna -1 se non è riuscito a scrivere nessun byte
      else{ break; }                                   //interrompe la scrittura e ritorna il numero di byte scritti
    }else if (nwritten == 0){                          //se ha scritto tutti i dati termina
			break;
		}
     nleft -= nwritten;                                //aggiorna il numero di byte da scrivere
     ptr   += nwritten;                                //aggiorna il numero di byte scritti
   }
   return(n - nleft);                                  //ritorna il numero di byte scritti
}



/* ----- send_to_coll
 *  Input:
 *   - *nome_file          nome del file da inviare al server collector
 *   - somma               risultato relativo a nome_file da inviare al server collector
 *   - HOST                indirizzo ip del serve collector
 *   - PORT                numero di porta sul quale è in ascolto il server collector
 *
 *  Output:
 *   - void
 *
 *  Descrizione:
 *   Invia al server collector il nome_file e il relativo valore
*/
void send_to_coll(char* nome_file, long valore, char* HOST, int PORT, int termina){

	int fd_skt = 0;                       //file descriptor associato al socket
	struct sockaddr_in serv_addr;         //oggetto per avviare la connessione
	ssize_t e;                            //prende il valore di ritorno della writen
	
  //crea il socket per la comunicazione
  if ((fd_skt = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		xtermina("Errore creazione socket", QUI);
	}
	
  //Setto le informazioni per avviare la connessione
  serv_addr.sin_family = AF_INET;                
  serv_addr.sin_port = htons(PORT);                  //converte la porta in formato network (short)
  serv_addr.sin_addr.s_addr = inet_addr(HOST);       //converte l'host nel formato adatto
	
  // apre connessione
	int prova = 0;
	while(prova < 3){
		if (connect(fd_skt, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
			fprintf(stderr,"\nErrore apertura connessione con il serve  (tentativo %d)\n",(prova+1));
			if(prova == 2){
			xtermina("E' stato impossibile collegarsi con il serve", QUI);
			}
			sleep(prova+2);
		  prova++;
		}
		else{
			break;
		}
	}


	//Comunica al serve di terminare                                    (inviando size = 0)
	if(termina == 1){
		int tmp = htonl(0);                                               //converto l'intero nel formato network
    e = writen(fd_skt,&tmp,sizeof(int));                              //chiamo la writen per inviare il segnale di terminazione
    if(e!=sizeof(int)) xtermina("Errore write in size", QUI);         //controlla di aver inviato correttamente
		return;
	}
	

	//Creo il messaggio da inviare
	char *str_valore = malloc(sizeof(char)*25);               //stringha che conterra valore
	char *msg = malloc(sizeof(char)*strlen(nome_file)+26);    //stringa che conterra nome_file+':'+str(valore)
	sprintf(str_valore, "%ld", valore);                       //converte valore in una stringa
	strcpy(msg,"");                                           //setta msg alla stringa vuota
	strcat(msg, nome_file);                                   //concatena nome_file a msg
	strcat(msg, ":");                                         //concatena '!' a msg
	strcat(msg, str_valore);                                  //concatena str(valore) a msg
	int size  = strlen(msg);                                  //size = lunghezza di msg
	int tmp;                                                  //variabile di supporto per fare la conversione nel formato network


	//Invia al server:  SIZE
	tmp = htonl(size);                                              //converte l'intero nel formato network
  e = writen(fd_skt,&tmp,sizeof(int));                            //chiama la writen per inviare size
  if(e!=sizeof(int)) xtermina("Errore write in size", QUI);       //controlla di aver inviato correttamente
	

	//Invia msg un carattere alla volta al server
	for(int i=0; i<size; i++){
		tmp = htonl((int)msg[i]);
  	e = writen(fd_skt,&tmp,sizeof(int));
  	if(e!=sizeof(int)) xtermina("Errore write", QUI);	
	}


	//Chiusura del socket
  if(close(fd_skt)<0){
		xtermina("Errore chiusura socket", QUI);           
	}


	//Stampa i valori inviati
	//fprintf(stderr,"\n  -> File: %s \tValore: %s", nome_file, str_valore);

	//Dealloca le stringhe
	free(str_valore);
	free(msg);
} 