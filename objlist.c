#define _POSIX_C_SOURCE 200112L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "objlist.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void Delete_Connection(ListaElementi * l, char * user_name){
    pthread_mutex_lock(&mutex);
    ListaElementi prec, corr;
    int trovato = 0;
    if(*l != NULL){
        if(strcmp((*l)->user_name, user_name) == 0){
            ListaElementi old = *l;
            *l = (*l)->next;
            free(old->user_name);
            free(old);
            trovato = 1;
        }
        else{
          prec = *l;
          corr = prec->next;
          while(corr != NULL && !trovato){
            if(strcmp(corr->user_name, user_name) == 0){
                trovato = 1;
                prec->next = corr->next;
                free(corr->user_name);
                free(corr);
            }
            else{
                prec = corr;
                corr = corr->next;
            }
          };
        }
     }
     pthread_mutex_unlock(&mutex);
}

void Delete_List(ListaElementi * l){
  if(*l){
    Delete_List(&(*l)->next);
    free((*l)->user_name);
    free(*l);
  }
}

int Is_Already_Conn(ListaElementi l, char * user_name){
  pthread_mutex_lock(&mutex);
  int trovato = 0;
  while(l != NULL && !trovato){
    if(strcmp(l->user_name, user_name) == 0)
      trovato = 1;
    else  l = l->next;
  }
  pthread_mutex_unlock(&mutex);
  return trovato;
}

int Number_Elem(ListaElementi l){
  int counter = 0;
  while(l != NULL){
    counter += 1;
    l = l->next;
  }
  return counter;
}

void Print_List(ListaElementi l){
    if(l != NULL){
        Print_List(l->next);
        printf("Utente : %s\n", l->user_name);
    }
}

void Set_Connection(ListaElementi * l, char * user_name){
    pthread_mutex_lock(&mutex);
    ListaElementi new = calloc((1),sizeof(ElementoLista));
    if(!new){
      perror("Allocazione nuovo elemento nella struttura");
      pthread_mutex_unlock(&mutex);
      exit(EXIT_FAILURE);
    }
    int len = strlen(user_name) + 1;
    new->user_name = (char*)calloc((len), sizeof(char));
    if(!new->user_name){
      perror("Allocazione user_name nella struttura");
      pthread_mutex_unlock(&mutex);
      if(new) free(new);
      exit(EXIT_FAILURE);
    }
    strcpy(new->user_name, user_name);
    new->next = *l;
    *l = new;
    pthread_mutex_unlock(&mutex);
}
