#define _POSIX_C_SOURCE 200112L
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "conn.h"
#include "objstore.h"
#include "objlist.h"

/*  NOTE : Questo file contiene commenti post consegna progetto per l'appello di settembre  */

/*  Variabili globali di utilita' */
ListaElementi Cond_List = NULL;
static int Stats_Array[5];
pthread_mutex_t mutex_stat = PTHREAD_MUTEX_INITIALIZER;
static volatile sig_atomic_t termina = 0;
static volatile sig_atomic_t stampa  = 0;

/*  Handler per la trattazione segnali  */
static void signalhandler(int sig);

/*  Esegue l'unlink del socket  */
void cleanup();

/*  Crea un thread per connessione  */
void spawn_thread(long connfd);

/*  Thread worker */
void *threadF(void *arg);

int main(int argc, char *argv[]) {
    Cond_List = NULL;

    memset(&Stats_Array, 0, sizeof(int));

    /*  Creazione socket di ascolto e accettazione connessioni  */
    cleanup();
    atexit(cleanup);

    /*  Preparazione per i segnali  */
    struct sigaction sa;
    memset (&sa, 0, sizeof(sa));
    sa.sa_handler = signalhandler;
    int notused;
    SYSCALL(notused, sigaction(SIGUSR1, &sa, NULL), "Sigaction SIGUSR1");
    SYSCALL(notused, sigaction(SIGINT, &sa, NULL), "Sigaction SIGINT");
    SYSCALL(notused, sigaction(SIGTERM, &sa, NULL), "Sigaction SIGTERM");

    int listenfd;
    SYSCALL(listenfd, socket(AF_UNIX, SOCK_STREAM, 0), "Socket");

    struct sockaddr_un serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME)+1);

    SYSCALL(notused, bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr)), "Bind");
    SYSCALL(notused, listen(listenfd, MAXBACKLOG), "Listen");

    /*  Server sempre in attesa, crea un thread per connessione e poi elabora i comandi ricevuti  */
    while(1){
    	long connfd;
      if ((connfd = accept(listenfd, (struct sockaddr*)NULL ,NULL)) == -1) {
         if (errno == EINTR) {
            if (termina) break;
         }
         else{
            perror("Accept");
            exit(EXIT_FAILURE);
         }
      }
    	spawn_thread(connfd);
    }

    if(stampa){
      int elementi_in_lista = Number_Elem(Cond_List);
      if(elementi_in_lista == 0)
        printf("Al momento di SIGUSR1 non c'era nessun client attivo\n");
      else{
        printf("Al momento di SIGUSR1 erano sempre attivi i seguenti client : \n");
        Print_List(Cond_List);
      }

      /*  Parte delle statistiche */
      char path1[14];
      char itoa[10];  //Ho dato 10 perche stavo pensando di scriverci la size della cartella data, poi ho utilizzato il comando bash.
      strcpy(path1,"./testout.log");
      int fd;
      if((fd = open(path1, O_CREAT | O_RDWR, 0600)) == -1){
        perror("S.C open in signalhandler");  //Qui signalhandler perche prima il file lo creavo dentro il signalhandler.
        exit(EXIT_FAILURE);
      }

      if(write(fd, "--- STATISTICHE CONCLUSIVE ---\n", strlen("--- STATISTICHE CONCLUSIVE ---\n")) == -1){
        perror("Write su testout.log");
        exit(EXIT_FAILURE);
      }

      if(write(fd, "Client collegati : \n", strlen("Client collegati : \n")) == -1){
        perror("Write su testout.log");
        exit(EXIT_FAILURE);
      }
      sprintf(itoa, "%d", Stats_Array[0]);
      if(write(fd, itoa, strlen(itoa)) == -1){
        perror("Write su testout.log");
        exit(EXIT_FAILURE);
      }

      if(write(fd, "\nNumero store andate a buon fine : \n", strlen("\nNumero store andate a buon fine : \n")) == -1){
        perror("Write su testout.log");
        exit(EXIT_FAILURE);
      }
      sprintf(itoa, "%d", Stats_Array[1]);
      if(write(fd, itoa, strlen(itoa)) == -1){
        perror("Write su testout.log");
        exit(EXIT_FAILURE);
      }
      if(write(fd, "/1000", strlen("/1000")) == -1){
        perror("Write su testout.log");
        exit(EXIT_FAILURE);
      }

      if(write(fd, "\nNumero retrieve andate a buon fine : \n", strlen("\nNumero retrieve andate a buon fine : \n")) == -1){
        perror("Write su testout.log");
        exit(EXIT_FAILURE);
      }
      sprintf(itoa, "%d", Stats_Array[2]);
      if(write(fd, itoa, strlen(itoa)) == -1){
        perror("Write su testout.log");
        exit(EXIT_FAILURE);
      }
      if(write(fd, "/600", strlen("/600")) == -1){
        perror("Write su testout.log");
        exit(EXIT_FAILURE);
      }

      if(write(fd, "\nNumero delete andate a buon fine : \n", strlen("\nNumero delete andate a buon fine : \n")) == -1){
        perror("Write su testout.log");
        exit(EXIT_FAILURE);
      }
      sprintf(itoa, "%d", Stats_Array[3]);
      if(write(fd, itoa, strlen(itoa)) == -1){
        perror("Write su testout.log");
        exit(EXIT_FAILURE);
      }
      if(write(fd, "/400", strlen("/400")) == -1){
        perror("Write su testout.log");
        exit(EXIT_FAILURE);
      }

      if(write(fd, "\nNumero di file contenuto nella cartella data : \n", strlen("\nNumero di file contenuto nella cartella data : \n")) == -1){
        perror("Write su testout.log");
        exit(EXIT_FAILURE);
      }
      sprintf(itoa, "%d", Stats_Array[4]);
      if(write(fd, itoa, strlen(itoa)) == -1){
        perror("Write su testout.log");
        exit(EXIT_FAILURE);
      }

      if(write(fd, "\nSize totale della cartella data : \n", strlen("\nSize totale della cartella data : \n")) == -1){
        perror("Write su testout.log");
        exit(EXIT_FAILURE);
      }
      stampa = 0;
    }

    /*  Deallocazione lista condivisa se necessario e chiusura lato server   */
    if(Cond_List) Delete_List(&Cond_List);
    return 0;
}

static void signalhandler(int sig){
  switch(sig){
    case SIGUSR1:{
      stampa = 1;
      termina = 1;
    } break;

    case SIGINT:{
      termina = 1;
    } break;

    case SIGTERM:{
      termina = 1;
    } break;

    default :{
      abort();
    }
  }
}

void cleanup(){
    unlink(SOCKNAME);
}

void *threadF(void *arg){
    assert(arg);
    long connfd = (long)arg;
    char nome_ut[128];
    do {
      char * operation = NULL;
      char * namee = NULL;
      char * path = NULL;
      void * blockk = NULL;
      char * name_file = NULL;

      /*    Ricevo Header da objstore   */
      msg_t header;
      int n;
      SYSCALL(n, readn(connfd, &header.len, sizeof(int)), "Read dim header");
      if (n==0) break;
      header.str = calloc((header.len+1), sizeof(char));
      if (header.str == NULL) {
        perror("Calloc header");
        break;
      }
      SYSCALL(n, readn(connfd, header.str, header.len * sizeof(char)), "Read full header");

      /*  Gestione comando STORE  */
      if(strncmp(header.str, "STORE", 5) == 0){
        /*  Preparazione campi per tokenizzazione */
        operation = (char*)calloc((6), sizeof(char));
        if(operation == NULL){
          perror("Calloc operation in os_store");
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_store");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write KO");
          Delete_Connection(&Cond_List, nome_ut);
          free(header.str);
          free(response.str);
          break;
        }

        /*  Tokenizzazione header */
        char * rest = header.str;
        strcpy(operation, strtok_r(rest, " ", &rest));
        name_file = (char*)calloc((1024), sizeof(char));
        if(name_file == NULL){
          perror("Calloc name in os_store");
          if(operation) free(operation);
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_store");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
          Delete_Connection(&Cond_List, nome_ut);
          free(header.str);
          free(response.str);
          break;
        }
        strcpy(name_file, strtok_r(rest, " ", &rest));
        size_t len = atoi(strtok_r(rest, "\n", &rest));
        blockk = calloc((len), sizeof(void));
        if(blockk == NULL){
          perror("Calloc block in os_store");
          if(operation) free(operation);
          if(name_file) free(name_file);
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_store");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
          Delete_Connection(&Cond_List, nome_ut);
          free(header.str);
          free(response.str);
          break;
        }
        strncpy((char*)blockk, rest, len);

        /*  Creazione path per creare il file */
        int len_name = strlen(nome_ut);
        int len_path = len_name + 7 + 1 + strlen(name_file);
        path = calloc((len_path+1), sizeof(char)); //Format : ./data/name
        if(path == NULL){
          perror("Calloc path in os_store");
          if(operation) free(operation);
          if(name_file) free(name_file);
          if(blockk)    free(blockk);
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_store");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
          Delete_Connection(&Cond_List, nome_ut);
          free(header.str);
          free(response.str);
          break;
        }
        strcpy(path,"./data/");
        strcat(path, nome_ut);
        strcat(path, "/");
        strcat(path, name_file);

        /*  Apertura File */
        int fd;
        if((fd = open(path, O_RDWR|O_CREAT, 0600)) == -1){
            perror("S.C. open in os_store");
            if(operation) free(operation);
            if(name_file) free(name_file);
            if(blockk)    free(blockk);
            if(path)      free(path);
            msg_t response;
            response.len = 3;
            response.str = calloc((response.len), sizeof(char));
            if(response.str == NULL){
              perror("Calloc response in os_store");
            }
            strcpy(response.str, "KO");

            /*  Risposta del server */
            SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
            SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
            Delete_Connection(&Cond_List, nome_ut);
            free(header.str);
            free(response.str);
            break;
        }

        /*  Storage blocco dati e aggiornamento contatore store */
        writen(fd, blockk, len);
        pthread_mutex_lock(&mutex_stat);
        Stats_Array[1] += 1;
        Stats_Array[4] += 1;
        pthread_mutex_unlock(&mutex_stat);

        /*  Chiusura file */
        if(close(fd) == -1){
          perror("Close in os_store");
          if(operation) free(operation);
          if(name_file) free(name_file);
          if(blockk)    free(blockk);
          if(path)      free(path);
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_store");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
          Delete_Connection(&Cond_List, nome_ut);
          free(header.str);
          free(response.str);
          break;
        }

        /*  invio ritorno positivo e deallocazione memoria */
        msg_t response;
        response.len = 3;
        response.str = calloc((response.len), sizeof(char));
        if(response.str == NULL){
          perror("Calloc response in os_store");
          Delete_Connection(&Cond_List, nome_ut);

          free(response.str);
          if(operation) free(operation);
          if(name_file) free(name_file);
          if(blockk)    free(blockk);
          if(path)      free(path);
          free(header.str);
          break;
        }

        strcpy(response.str, "OK");

        /*  Risposta del server */
        SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim OK");
        SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data OK");
        free(response.str);

        if(operation) free(operation);
        if(name_file) free(name_file);
        if(blockk)    free(blockk);
        if(path)      free(path);
      }

      /*  Gestione comando LEAVE  */
      if(strncmp(header.str, "LEAVE", 5) == 0){
        /*  Setting array e lista per le statistiche  */
        pthread_mutex_lock(&mutex_stat);
        Stats_Array[0] = Stats_Array[0] - 1;
        pthread_mutex_unlock(&mutex_stat);
        Delete_Connection(&Cond_List, nome_ut);

        /*  La leave risponde sempre OK */
        msg_t response;
        response.len = 3;
        response.str = calloc((response.len), sizeof(char));
        if(response.str == NULL){
          perror("Calloc response in os_disconnect");
          free(response.str);
          free(header.str);
          break;
        }
        strcpy(response.str, "OK");

        /*  Risposta del server */
        SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim OK");
        SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data OK");

        free(response.str);
      }

      /*  Gestione comando DELETE */
      if(strncmp(header.str, "DELETE", 6) == 0){
        /*  Preparazione campi per la tokenizzazione  */
        int len_name = strlen(header.str) - 8;
        name_file = calloc((len_name+1), sizeof(char));
        if(name_file == NULL){
          perror("Calloc name per l'eliminazione del file");
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_delete");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
          Delete_Connection(&Cond_List, nome_ut);
          free(header.str);
          free(response.str);
          break;
        }
        operation = calloc((7), sizeof(char));
        if(operation == NULL){
          perror("Calloc operation per l'eliminazione del file");
          if(name_file) free(name_file);
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_delete");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
          Delete_Connection(&Cond_List, nome_ut);
          free(header.str);
          free(response.str);
          break;
        }

        /*  Tokenizzazione dell Header  */
        char * rest = header.str;
        strcpy(operation, strtok_r(rest, " ", &rest));
        strcpy(name_file, strtok_r(rest, "\n", &rest));

        /*  Creazione path per la cancellazione file */
        int len_name2 = strlen(nome_ut);
        int len_path = len_name2 + 7 + 1 + strlen(name_file);
        path = calloc((len_path+1), sizeof(char)); //Format : ./data/name
        if(path == NULL){
          perror("Calloc path in os_delete");
          if(name_file) free(name_file);
          if(operation) free(operation);
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_delete");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
          Delete_Connection(&Cond_List, nome_ut);
          free(header.str);
          free(response.str);
          break;
        }
        strcpy(path, "./data/");
        strcat(path, nome_ut);
        strcat(path, "/");
        strcat(path, name_file);

        /*  Cancellazione file  */
        if((unlink(path)) == -1){
          perror("Errore cancellazione file");
          if(name_file) free(name_file);
          if(operation) free(operation);
          if(path)      free(path);
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_delete");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
          Delete_Connection(&Cond_List, nome_ut);
          free(header.str);
          free(response.str);
          break;
        }

        /*  Aggiornamento contatore delete per statistiche  */
        pthread_mutex_lock(&mutex_stat);
        Stats_Array[3] += 1;
        Stats_Array[4] -= 1;
        pthread_mutex_unlock(&mutex_stat);

        /*  Invio ritorno positivo e deallocazione memoria  */
        msg_t response;
        response.len = 3;
        response.str = calloc((response.len), sizeof(char));
        if(response.str == NULL){
          perror("Calloc response in os_delete");

          Delete_Connection(&Cond_List, nome_ut);
          free(header.str);
          free(response.str);
          if(name_file) free(name_file);
          if(operation) free(operation);
          if(path)      free(path);
          break;
        }

        strcpy(response.str, "OK");

        /*  Risposta del server */
        SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim OK");
        SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data OK");

        free(response.str);
        if(name_file) free(name_file);
        if(operation) free(operation);
        if(path)      free(path);
      }

      /*  Gestione comando REGISTER */
      if(strncmp(header.str, "REGISTER", 8) == 0){
        /*  Preparazione campi per tokenizzazione  */
        msg_t response_ok;
        int len_name = strlen(header.str) - 10; //Tolgo il comando, lo spazio e \n
        namee = calloc((len_name+1), sizeof(char));
        if(namee == NULL){
          perror("Calloc name per creazione directory");
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_connect");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
          free(header.str);
          free(response.str);
          break;
        }
        operation = calloc((9), sizeof(char));
        if(operation == NULL){
          perror("Calloc operation pre tokenizzazione in os_connect");
          if(namee) free(namee);
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_connect");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
          free(header.str);
          free(response.str);
          break;
        }

        /*  Tokenizzazione dell header  */
        char * rest = header.str;
        strcpy(operation, strtok_r(rest, " ", &rest));
        strcpy(namee, strtok_r(rest, "\n", &rest));
        strcpy(nome_ut, namee);

        /*  Creazione path per creare cartella  */
        int len_path = strlen(namee) + 7;
        char * path = (char*)calloc((len_path+1), sizeof(char));
        if(path == NULL){
          perror("Calloc path in os_connect");
          if(namee) free(namee);
          if(operation) free(operation);
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_connect");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
          free(header.str);
          free(response.str);
          break;
        }
        strcpy(path,"./data/");
        strcat(path, namee);

        /*  Controllo se cé gia una doppia connessione  */
        int double_conn = Is_Already_Conn(Cond_List, nome_ut);
        if(double_conn == 1){
          perror("Client gia collegato");
          if(namee) free(namee);
          if(operation) free(operation);
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_connect");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
          free(header.str);
          free(response.str);
          break;
        }

        /*  Creazione directory con nome passato dal client, 0777 permessi RWX */
        if(mkdir(path, 0777) == -1){
          int errsv = errno;
          if(errsv == EEXIST){
            goto dontstop;
          }
          else{
            perror("Mkdir in os_connect");
            if(namee) free(namee);
            if(operation) free(operation);
            if(path)  free(path);
            msg_t response;
            response.len = 3;
            response.str = calloc((response.len), sizeof(char));
            if(response.str == NULL){
              perror("Calloc response in os_connect");
            }
            strcpy(response.str, "KO");

            /*  Risposta del server */
            SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
            SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
            free(header.str);
            free(response.str);
            break;
          }
        }

dontstop:
        response_ok.len = 3;
        response_ok.str = calloc((response_ok.len), sizeof(char));
        if(response_ok.str == NULL){
          perror("Calloc response in os_connect");

          free(header.str);
          free(response_ok.str);
          if(operation) free(operation);
          if(namee)     free(namee);
          if(path)      free(path);
          break;
        }

        /*  Aggiorno il contatore client connessi e setto il client nella lista */
        pthread_mutex_lock(&mutex_stat);
        Stats_Array[0] += 1;
        pthread_mutex_unlock(&mutex_stat);
        Set_Connection(&Cond_List, nome_ut);

        strcpy(response_ok.str, "OK");

        /*  Risposta del server */
        SYSCALL(n, writen(connfd, &response_ok.len, sizeof(int)), "Write dim OK");
        SYSCALL(n, writen(connfd, response_ok.str, response_ok.len*sizeof(char)), "Write data OK");

        free(response_ok.str);
        if(operation) free(operation);
        if(namee)     free(namee);
        if(path)      free(path);
      }

      /*  Gestione comando RETRIEVE */
      if(strncmp(header.str, "RETRIEVE", 8) == 0){
        /*  Preparazione campi per la tokenizzazione  */
        int len_name = strlen(header.str) - 10;
        name_file = calloc((len_name+1), sizeof(char));
        if(name_file == NULL){
          perror("Calloc name per la restituzione oggetto");
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_retrieve");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
          Delete_Connection(&Cond_List, nome_ut);
          free(header.str);
          free(response.str);
          break;
        }
        operation = calloc((9), sizeof(char));
        if(operation == NULL){
          perror("Calloc operation per la restituzione oggetto");
          if(name_file) free(name_file);
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_retrieve");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
          Delete_Connection(&Cond_List, nome_ut);
          free(header.str);
          free(response.str);
          break;
        }

        /*  Tokenizzazione dell header  */
        char * rest = header.str;
        strcpy(operation, strtok_r(rest, " ", &rest));
        strcpy(name_file, strtok_r(rest, "\n", &rest));

        /*  Creazione path per aprire la cartella giusta  */
        int len_path = strlen(nome_ut) + 7 + 1 + strlen(name_file);
        path = calloc((len_path+1), sizeof(char)); //Format : ./data/name
        if(path == NULL){
          perror("Calloc path in os_retrieve");
          if(name_file) free(name_file);
          if(operation) free(operation);
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_retrieve");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
          Delete_Connection(&Cond_List, nome_ut);
          free(header.str);
          free(response.str);
          break;
        }
        strcpy(path, "./data/");
        strcat(path, nome_ut);
        strcat(path, "/");
        strcat(path, name_file);

        /*  Apertura file e lettura contenuto salvandolo in una variabile */
        struct stat info;
        if(stat(path, &info) == -1){
          perror("Errore in cercare di trovare la dimensione del file in os_retrieve");
          if(name_file) free(name_file);
          if(operation) free(operation);
          if(path)      free(path);
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_retrieve");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
          Delete_Connection(&Cond_List, nome_ut);
          free(header.str);
          free(response.str);
          break;
        }
        int len = info.st_size+1;
        blockk = calloc((len), sizeof(void));
        if(blockk == NULL){
          perror("Calloc block per la restituzione dell oggetto");
          if(name_file) free(name_file);
          if(operation) free(operation);
          if(path)      free(path);
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_retrieve");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
          Delete_Connection(&Cond_List, nome_ut);
          free(header.str);
          free(response.str);
          break;
        }
        int fd, lung;
        if((fd = open(path, O_RDONLY)) == -1){
          perror("Errore apertura file per la restituzione dell oggetto");
          if(name_file) free(name_file);
          if(operation) free(operation);
          if(path)      free(path);
          if(blockk)    free(blockk);
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_retrieve");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
          Delete_Connection(&Cond_List, nome_ut);
          free(header.str);
          free(response.str);
          break;
        }
        while((lung = read(fd, blockk, (int)info.st_size)) > 0);
        if(lung == -1){
          perror("Errore lettura del file per la restituzione dell oggetto");
          if(name_file) free(name_file);
          if(operation) free(operation);
          if(path)      free(path);
          if(blockk)    free(blockk);
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_retrieve");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
          Delete_Connection(&Cond_List, nome_ut);
          free(header.str);
          free(response.str);
          break;
        }

        /*  Chiusura file e invio Ritorno */
        if(close(fd) == -1){
          perror("Errore chiusura file per la restituzione dell oggetto");
          if(name_file) free(name_file);
          if(operation) free(operation);
          if(path)      free(path);
          if(blockk)    free(blockk);
          msg_t response;
          response.len = 3;
          response.str = calloc((response.len), sizeof(char));
          if(response.str == NULL){
            perror("Calloc response in os_retrieve");
          }
          strcpy(response.str, "KO");

          /*  Risposta del server */
          SYSCALL(n, writen(connfd, &response.len, sizeof(int)), "Write dim KO");
          SYSCALL(n, writen(connfd, response.str, response.len*sizeof(char)), "Write data KO");
          Delete_Connection(&Cond_List, nome_ut);
          free(header.str);
          free(response.str);
          break;
        }

        /*  Risposta del server, caso OK */
        void * response;
        char * tmp_itoa = calloc((7), sizeof(char));
        sprintf(tmp_itoa, "%d", (int)info.st_size);
        int len_block = strlen((char*)blockk) + 7 + strlen(tmp_itoa);
        response = calloc((len_block+1), sizeof(void));
        if(!response){
          perror("Calloc response in os_retrieve");
          Delete_Connection(&Cond_List, nome_ut);

          free(header.str);
          free(response);
          if(tmp_itoa)  free(tmp_itoa);
          if(name_file) free(name_file);
          if(operation) free(operation);
          if(path)      free(path);
          if(blockk)    free(blockk);
          break;
        }

        /*  Preparazione e invio header da protocollo DATA len\n blocco_dati  */
        strcat((char*)response, "DATA ");
        strcat((char*)response, tmp_itoa);
        strcat((char*)response, "\n");
        strcat((char*)response, (char*)blockk);

        SYSCALL(n, writen(connfd, &len_block, sizeof(int)), "Write dim block");
        SYSCALL(n, writen(connfd, response, len_block*sizeof(char)), "Write data block");

        pthread_mutex_lock(&mutex_stat);
        Stats_Array[2] += 1;
        pthread_mutex_unlock(&mutex_stat);

        if(response)  free(response);
        if(tmp_itoa)  free(tmp_itoa);
        if(name_file) free(name_file);
        if(operation) free(operation);
        if(path)      free(path);
        if(blockk)    free(blockk);
      }

      if(header.str) free(header.str);
    } while(1);
    close(connfd);
    return NULL;
}

void spawn_thread(long connfd){
    pthread_attr_t thattr;
    pthread_t thid;

    sigset_t mask,oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    if (pthread_sigmask(SIG_BLOCK, &mask, &oldmask) != 0) {
      fprintf(stderr, "FATAL ERROR\n");
      close(connfd);
      return;
    }

    if (pthread_attr_init(&thattr) != 0) {
    	fprintf(stderr, "pthread_attr_init FALLITA\n");
    	close(connfd);
    	return;
    }

    if (pthread_attr_setdetachstate(&thattr, PTHREAD_CREATE_DETACHED) != 0) {
    	fprintf(stderr, "pthread_attr_setdetachstate FALLITA\n");
    	pthread_attr_destroy(&thattr);
    	close(connfd);
    	return;
    }
    if (pthread_create(&thid, &thattr, threadF, (void*)connfd) != 0) {
    	fprintf(stderr, "pthread_create FALLITA");
    	pthread_attr_destroy(&thattr);
    	close(connfd);
    	return;
    }

    if (pthread_sigmask(SIG_SETMASK, &oldmask, NULL) != 0) {
      fprintf(stderr, "FATAL ERROR\n");
      close(connfd);
      return;
    }
}
