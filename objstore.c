#define _POSIX_C_SOURCE 200112L
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "objstore.h"
#include "conn.h"

/*  FUNZIONI DA IMPLEMENTARE

  * int os_connect(char * name);  //Inizia la connessione all ojstore, registrando il cliente con il NAME dato
  * int os_store(char * name, void * block, size_t len); //Richiede all'object store la memorizzazione dell'oggetto puntato da block, per una lunghezza len, con il nome name
  * void *os_retrieve(char * name); //Recupera dall'object store l'oggetto precedentemente memorizzato sotto il nome name.
  * int os_delete(char * name); //Cancellla l'oggetto di nome name precedentemente memorizzato.
  * int os_disconnect();  //Chiude la connessione all ojstore.

*/

struct sockaddr_un serv_addr;
int sockfd, notused;
char * buffer = NULL;

int os_connect(char * name){
  /*  Creazione socket  */
  SYSCALL(sockfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket");
  memset(&serv_addr, '0', sizeof(serv_addr));

  /*  Connessione al server */
  serv_addr.sun_family = AF_UNIX;
  strncpy(serv_addr.sun_path,SOCKNAME, strlen(SOCKNAME)+1);
  SYSCALL(notused, connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)), "connect");

  /*  Preparazione header da inviare  */
  int len_head = strlen("REGISTER") + 1 + strlen(name) + 1; //Format : REGISTER name\n
  char * request = calloc((len_head), sizeof(char));
  if(request == NULL){
    perror("Calloc request in os_connect");
    close(sockfd);
    return FALSE;
  }
  strcat(request, "REGISTER");
  strcat(request, " ");
  strcat(request, name);
  request[len_head-1] = '\n';

  /*  Invio header al server  */
  SYSCALL(notused, writen(sockfd, &len_head, sizeof(int)), "Write dim in os_connect");
  SYSCALL(notused, writen(sockfd, request, len_head*sizeof(char)), "Write data in os_connect");

  /*  Gestione ritorno da parte del server  */
  int n = 3;
  int ret_value;
  buffer = calloc((n), sizeof(char));
  if (buffer == NULL) {
      perror("Calloc buffer in os_connect");
      if(request) free(request);
      close(sockfd);
      return FALSE;
  }
  SYSCALL(notused, readn(sockfd, &n, sizeof(int)), "Read dim in os_connect");
  SYSCALL(notused, readn(sockfd, buffer, n*sizeof(char)), "Read data in os_connect");
  buffer[n-1] = '\0';
  if(strncmp(buffer, "OK", 2) == 0)
    ret_value = TRUE;
  else{
    ret_value = FALSE;
    close(sockfd);
  }

  /*  Deallocazione memoria e invio risultato(T/F)  */
  if(buffer) free(buffer);
  if(request) free(request);
  return ret_value;
}

int os_store(char * name, void * block, size_t len){
  /*  Preparazione header da inviare  */
  int len_head = strlen(name) + len + 17; //Format : STORE name len\n block\n
  char * request = calloc((len_head), sizeof(char));
  if(request == NULL){
    perror("Calloc header in os_store");
    close(sockfd);
    return FALSE;
  }
  strcat(request, "STORE");
  strcat(request, " ");
  strcat(request, name);
  strcat(request, " ");
  char * str = calloc((7), sizeof(char));
  sprintf(str, "%zu", len); //Per la conversione da unsignedlong in char*
  strcat(request, str);
  strcat(request, "\n");
  strcat(request, block);
  strcat(request, "\n");

  /*  Invio header al server  */
  SYSCALL(notused, writen(sockfd, &len_head, sizeof(int)), "Write dim in os_store");
  SYSCALL(notused, writen(sockfd, request, len_head*sizeof(char)), "Write data in os_store");

  /*  Gestione ritorno da parte del server  */
  int n = 3;
  int ret_value;
  buffer = calloc((n), sizeof(char));
  if (buffer == NULL) {
      perror("Calloc buffer in os_store");
      close(sockfd);
      if(request) free(request);
      if(str)     free(str);
      return FALSE;
  }
  SYSCALL(notused, readn(sockfd, &n, sizeof(int)), "Read dim in os_store");
  SYSCALL(notused, readn(sockfd, buffer, n*sizeof(char)), "Read data in os_store");
  buffer[n-1] = '\0';
  if(strncmp(buffer, "OK", 2) == 0)
    ret_value = TRUE;
  else{
    ret_value = FALSE;
    close(sockfd);
  }

  /*  Deallocazione memoria e invio risultato(T/F)  */
  if(buffer) free(buffer);
  if(request) free(request);
  if(str) free(str);
  return ret_value;
}

void *os_retrieve(char * name){
  /*  Preparazione header da inviare  */
  int len_head = strlen("RETRIEVE") + strlen(name) + 2; //Format : RETRIVE name\n;
  char * request = NULL;
  request = calloc((len_head), sizeof(char));
  if(request == NULL){
    perror("Calloc request in os_retrieve");
    close(sockfd);
    return NULL;
  }
  strcat(request, "RETRIEVE");
  strcat(request, " ");
  strcat(request, name);
  request[len_head-1] = '\n';

  /*  Invio header al server  */
  SYSCALL(notused, writen(sockfd, &len_head, sizeof(int)), "Write dim in os_retrieve");
  SYSCALL(notused, writen(sockfd, request, len_head*sizeof(char)), "Write data in os_retrieve");
  if(request)  free(request);

  /*  Gestione ritorno da parte del server  */
  int n = 0;
  SYSCALL(notused, readn(sockfd, &n, sizeof(int)), "Read dim in os_retrieve");
  void * buffer2 = NULL;
  buffer2 = calloc((n), sizeof(void));
  if(buffer2 == NULL){
    perror("Calloc buffer in os_retrieve");
    close(sockfd);
    return NULL;
  }
  SYSCALL(notused, readn(sockfd, buffer2, n*sizeof(char)), "Read data in os_retrieve");

  /*  Controllo se ho ricevuto un KO  */
  if(strncmp((char*)buffer2, "KO", 2) == 0){
    if(buffer2) free(buffer2);
    close(sockfd);
    return NULL;
  }

  /*  Preparazione variabili per tokenizzazione */
  char * rest = buffer2;
  char * notinf;
  size_t len;
  void * blocco;

  notinf = (char*)calloc((5), sizeof(char));
  if(!notinf){
    perror("Allocazione notinf in os_retrieve");
    if(buffer2) free(buffer2);
    close(sockfd);
    return NULL;
  }
  strcpy(notinf, strtok_r(rest, " ", &rest));
  len = atoi(strtok_r(rest, "\n", &rest));
  blocco = calloc((len+1), sizeof(void));
  if(!blocco){
    perror("Allocazione blocco in os_retrieve");
    if(buffer2) free(buffer2);
    if(notinf)  free(notinf);
    close(sockfd);
    return NULL;
  }
  strncpy((char*)blocco, rest, len);

  /*  Deallocazione memoria invio blocco dati */
  if(buffer2) free(buffer2);
  if(notinf)  free(notinf);
  return blocco;
}

int os_delete(char * name){
  /*  Preparazione header da inviare  */
  int len_head = strlen("DELETE") + strlen(name) + 2; //Format : DELETE name\n
  char * request = NULL;
  request = calloc((len_head), sizeof(char));
  if(request == NULL){
    perror("Calloc request in os_delete");
    close(sockfd);
    return FALSE;
  }
  strcat(request, "DELETE");
  strcat(request, " ");
  strcat(request, name);
  request[len_head-1] = '\n';

  /*  Invio header al Server  */
  SYSCALL(notused, writen(sockfd, &len_head, sizeof(int)), "Write dim in os_delete");
  SYSCALL(notused, writen(sockfd, request, len_head*sizeof(char)), "Write data in os_delete");

  /*  Gestione ritorno da parte del server  */
  int n = 3;
  int ret_value;
  buffer = calloc((n), sizeof(char));
  if (buffer == NULL) {
      perror("Calloc buffer in os_delete");
      if(request) free(request);
      close(sockfd);
      return FALSE;
  }
  SYSCALL(notused, readn(sockfd, &n, sizeof(int)), "Read dim in os_delete");
  SYSCALL(notused, readn(sockfd, buffer, n*sizeof(char)), "Read data in os_delete");
  buffer[n-1] = '\0';
  if(strncmp(buffer, "OK", 2) == 0)
    ret_value = TRUE;
  else{
    ret_value = FALSE;
    close(sockfd);
  }

  /*  Deallocazione memoria e invio risultato(T/F)  */
  if(buffer) free(buffer);
  if(request) free(request);
  return ret_value;
}

int os_disconnect(){
  /*  Preparazione header da inviare  */
  int len_head = strlen("LEAVE") + 1; //Format : LEAVE\n
  char * request = calloc((len_head), sizeof(char));
  if(request == NULL){
    perror("Calloc request in os_disconnect");
    close(sockfd);
    return FALSE;
  }
  strncpy(request, "LEAVE\n", len_head);

  /*  Invio header al server  */
  SYSCALL(notused, writen(sockfd, &len_head, sizeof(int)), "Write dim in os_disconnect");
  SYSCALL(notused, writen(sockfd, request, len_head*sizeof(char)), "Write data in os_disconnect");

  /*  Restituisce sempre ok la leave */
  int n = 3;
  buffer = calloc((n), sizeof(char));
  if(!buffer){
    perror("Calloc buffer in os_disconnect");
    if(request)  free(request);
    close(sockfd);
    return FALSE;
  }

  SYSCALL(notused, readn(sockfd, &n, sizeof(int)), "Read dim in os_disconnect");
  SYSCALL(notused, readn(sockfd, buffer, n*sizeof(char)), "Read data in os_disconnect");
  buffer[n-1] = '\0';

  /*  Chiusura socket */
  close(sockfd);

  /*  Deallocazione memoria e invio risultato */
  if(buffer) free(buffer);
  if(request) free(request);
  return TRUE;
}
