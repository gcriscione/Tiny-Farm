#! /usr/bin/env python3
# server che si mette in ascolto nel socket con HOST e PORTA passati come parametri,  
# riceve le coppie (nome_file, valore) dai thread-worker e le stampa a video
# termina quando riceve la coppia ("", 0)

import sys, struct, socket, threading, os, signal
# host e porta di default
host = "127.0.0.1"        # Indirizzo ip                      (localhost)
port = 57451              # Porta su cui stare in ascolto     (non-privileged ports are > 1023)


''' ----- ClientThread
 *  Input:
 *   - threading.Thread                (indica che è una sottoclasse di threading.Thread)
 *
 *  Output:
 *   - void
 *
 *  Descrizione:
 *   sottoclasse della classe thread che lancia gestisci_connessione
'''
# codice da eseguire nei singoli thread 
class ClientThread(threading.Thread):
    def __init__(self,conn,addr):            #costruttire
        threading.Thread.__init__(self)      #richiama il costruttore della superclasse
        self.conn = conn                     #setta conn
        self.addr = addr                     #setta addr

		#Override
    def run(self):
      #print("\n", self.ident, "sto gestendo la connessione con", self.addr,"\n")
      gestisci_connessione(self.conn,self.addr)           #invoca la funzione per gestire la singola connessione
      #print("\n", self.ident, "connessione chiusa\n")



''' ----- main
 *  Input:
 *   - HOST                indirizzo ip del client farm
 *   - PORT                numero di porta sul quale è in ascolto il server collector
 *
 *  Output:
 *   - void
 *
 *  Descrizione:
 *   gestise le connessioni dei thread-worker
'''
def main():
  global termina                     #indica che termini è una variabile globale
	
  #crea il socket
  threads = []
  with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    try:  
      s.bind((host, port))            #associa al socket l'indirzzo ip e la porta
      s.listen()                      #prepara il server alla ricezione di dati
      s.settimeout(4)
      print("Server attivo, in attesa di un client...\n")
      while(termina == 0):
        #print("\nAttendo ",str(termina))
        try:
          conn, addr = s.accept()       #si blocca finchè non arriva una richiesta da client
        except socket.timeout:
          continue
				
        t = ClientThread(conn,addr)   #creo un thread che si occupa di gestire la connessione
        t.start()                     #(invoca il metodo run)
        threads.append(t)
				
      # join all threads
      for t in threads:
        t.join()
        
    except KeyboardInterrupt:
      pass
			
    s.shutdown(socket.SHUT_RDWR)
    print('\n\n--- Server Terminato ---')




''' ----- gestisci_connessione
 *  Input:
 *   - conn                oggetto che rappresenta la connessione
 *   - addr                indirizzo ip del client che ha contattato il client
 *
 *  Output:
 *   - void
 *
 *  Descrizione:
 *   gestise una singola connessione, ricevendo i dati inviati dai thread-worker
'''
def gestisci_connessione(conn,addr):
  global termina                     #indica che termini è una variabile globale
	
  with conn:  
		
    # attende un intero che indica la size in byte della successiva stringa da ricevere
    data = recv_all(conn,4)                     #legge dal socket 4 bytes
    assert len(data)==4                         #verifica di aver letto tutti i bytes
    size  = struct.unpack("!i",data)[0]         #decodifica i dati ricevuti in un intero a 4 byte

    if(size == 0):                              #termina il server quando riceve una stringa vuota
      termina = 1;                              #setta la variabile globale a 1 in modo da terminare il ciclo nel main
      return                                    #termina la funzione senza stampare i valori
			
    str_data = ""
    for i in range(size):
      data = recv_all(conn,4)
      assert len(data)==4
      str_data += chr(struct.unpack("!i",data)[0])

    nome_file, somma = str_data.split(':');
    print("\n"+somma+" "+nome_file, end="");

		
''' ----- recv_all
 *  Input:
 *   - conn                oggetto che rappresenta la connessione
 *   - n                   numero di byte da leggere
 *
 *  Output:
 *   - chunks              array di byte contenente i valore letti dal socket
 *
 *  Descrizione:
 *   resta in attesa finchè non riceve esattamente n bytes e li restituisce
'''
def recv_all(conn,n):
  chunks = b''            #inizializzazione array di byte
  bytes_recd = 0          #conta quanti byte sono stati letti

	#Ripete finché non legge tutti i byte
  while bytes_recd < n:
    chunk = conn.recv(min(n - bytes_recd, 1024))         #legge dal socket
    if len(chunk) == 0:
      raise RuntimeError("socket connection broken")     #errore se non è riuscito a leggere niente
    chunks += chunk                                      #aggiorna l'array di byte
    bytes_recd = bytes_recd + len(chunk)                 #aggiorna il numero di byte letti
  return chunks




	
#Controlla il numero di argomenti e chiama la funzione main() passando gli agrmoneti forniti

termina = 0             # variabile globale utilizzata nel main che indica quando terminare il server


if len(sys.argv)==1:
  main()
else:
  print("Uso:\n\t %s [host] [port]" % sys.argv[0])