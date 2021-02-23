#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "objstore.h"

char block[100];
size_t dim_block = 100;

char * eat_sleep_string_repeat(int i){
  char * rpt = (char*)calloc(dim_block * i, sizeof(char));
  size_t seek = 0;
  while(seek < dim_block * i - 100){
    memcpy(rpt + seek, block, 100);
    seek = seek + dim_block;
  }
  memcpy(rpt + seek, block, 99);
  return rpt;
}

int main(int argc, char * argv[]){
  if(argc != 3){
    perror("Parametri sbagliati");
    exit(EXIT_FAILURE);
  }

  /*  Preparazione variabili per il testing */
  int count_store = 0;
  int count_retrieve = 0;
  int count_delete = 0;
  char * user_name = NULL;
  int check_result = 0, dim_usrname, i, wich_test;
  wich_test = atoi(argv[2]);
  dim_usrname = strlen(argv[1])+1;
  if(wich_test > 3 || wich_test < 1){
    perror("Test sconosciuto (test abilitati : 1,2,3)");
    exit(EXIT_FAILURE);
  }

  /*  Allocazione variabili per il testing  */
  user_name = (char*)calloc((dim_usrname), sizeof(char));
  if(!user_name){
    perror("Allocazione user_name");
    exit(EXIT_FAILURE);
  }
  strcpy(user_name, argv[1]);
  strncpy(block, "Lorem ipsum dolor sit amet, consectetur adipisci elit, sed do eiusmod tempor incidunt ut labore....\n", dim_block);

  /*  Testing delle funzioni della libreria objstore  */
  if(wich_test == 1){
    if((check_result = os_connect(user_name)) == 1){
      perror("KO os_connect");
      if(user_name) free(user_name);
      exit(EXIT_FAILURE);
    }

    for(i = 1; i <= 20; i++){
      char * file = NULL;
      file = (char*)calloc((8), sizeof(char));
      if(!file){
        perror("Allocazione nome file");
        if(user_name) free(user_name);
        exit(EXIT_FAILURE);
      }
      strcpy(file, "Test_");
      char * itoa = calloc((3), sizeof(char));
      if(!itoa){
        perror("Allocazione itoa");
        if(user_name) free(user_name);
        if(file)      free(file);
        exit(EXIT_FAILURE);
      }
      sprintf(itoa, "%d", i);
      strcat(file, itoa);

      if(i == 1){
          if((check_result = os_store(file, block, dim_block)) == 1){
              perror("KO os_store");
              if(user_name) free(user_name);
              if(file)      free(file);
              exit(EXIT_FAILURE);
          }
          count_store += 1;
          if(file)  free(file);
          if(itoa) free(itoa);
      }
      else{
          /*  Dimensione crescente della stringa */
          char * str = eat_sleep_string_repeat(i * 50);

          if((check_result = os_store(file, str, dim_block * i * 50)) == 1){
            perror("KO os_store");
            if(user_name) free(user_name);
            if(file)      free(file);
            exit(EXIT_FAILURE);
          }
          count_store += 1;

          if(file)  free(file);
          if(str)  free(str);
          if(itoa) free(itoa);
      }
    }

    int notinf;
    if((notinf = os_disconnect(user_name)) != 0){
      perror("KO os_disconnect");
      if(user_name) free(user_name);
      exit(EXIT_FAILURE);
    }
  }

  if(wich_test == 2){
    if((check_result = os_connect(user_name)) == 1){
      perror("KO os_connect");
      if(user_name) free(user_name);
      exit(EXIT_FAILURE);
    }

    for(i = 1; i <= 20; i++){
      char * file = NULL;
      file = (char*)calloc((50), sizeof(char));
      if(!file){
        perror("Allocazione nome file");
        if(user_name) free(user_name);
        exit(EXIT_FAILURE);
      }
      strcat(file, "Test_");
      char * itoa = calloc((3), sizeof(char));
      if(!itoa){
        perror("Allocazione itoa");
        if(user_name) free(user_name);
        exit(EXIT_FAILURE);
      }
      sprintf(itoa, "%d", i);
      strcat(file, itoa);
      void * tmp = os_retrieve(file);
      if(tmp == NULL){
          perror("KO os_retrieve");
          if(user_name) free(user_name);
          if(itoa)  free(itoa);
          if(file)  free(file);
          exit(EXIT_FAILURE);
      }

      /*  Controllo se cio che mi e'stato restituito sia corretto */
      if(i == 1){
        if(strncmp((char*)tmp, block, strlen((char*)tmp)) != 0){
          perror("KO os_retrieve, diversa restituzione");
          if(user_name) free(user_name);
          if(itoa)  free(itoa);
          if(file)  free(file);
          if(tmp)   free(tmp);
          /*  In caso di fallimento comunque faccio scollegare il client  */
          int notinf;
          if((notinf = os_disconnect(user_name)) != 0){
            perror("KO os_disconnect");
            if(user_name) free(user_name);
          }
          exit(EXIT_FAILURE);
        }
      }

      else{
        char * str = eat_sleep_string_repeat(i * 50);
        if(strncmp((char*)tmp, str, strlen(str)) != 0){
          perror("KO os_retrieve, diversa restituzione 2");
          if(user_name) free(user_name);
          if(itoa)  free(itoa);
          if(file)  free(file);
          if(tmp)   free(tmp);
          if(str)   free(str);
          int notinf;
          /*  In caso di fallimento comunque faccio scollegare il client  */
          if((notinf = os_disconnect(user_name)) != 0){
            perror("KO os_disconnect");
            if(user_name) free(user_name);
          }
          exit(EXIT_FAILURE);
        }
        if(str) free(str);
      }

      count_retrieve += 1;
      if(file) free(file);
      if(tmp)  free(tmp);
      if(itoa) free(itoa);
    }

    int notinf;
    if((notinf = os_disconnect(user_name)) != 0){
      perror("KO os_disconnect");
      if(user_name) free(user_name);
      exit(EXIT_FAILURE);
    }
  }

  if(wich_test == 3){
    if((check_result = os_connect(user_name)) == 1){
      perror("KO os_connect");
      if(user_name) free(user_name);
      exit(EXIT_FAILURE);
    }

    for(i = 1; i <= 20; i++){
      char * file = NULL;
      file = (char*)calloc((8), sizeof(char));
      if(!file){
        perror("Allocazione nome file");
        if(user_name) free(user_name);
        exit(EXIT_FAILURE);
      }
      strcpy(file, "Test_");
      char * itoa = calloc((3), sizeof(char));
      if(!itoa){
        perror("Allocazione itoa");
        if(user_name) free(user_name);
        if(file)      free(file);
        exit(EXIT_FAILURE);
      }
      sprintf(itoa, "%d", i);
      strcat(file, itoa);
      if((check_result = os_delete(file)) == 1){
        perror("KO os_delete");
        if(user_name) free(user_name);
        if(itoa)      free(itoa);
        if(file)      free(file);
        exit(EXIT_FAILURE);
      }
      count_delete += 1;
      if(file) free(file);
      if(itoa) free(itoa);
    }

    int notinf;
    if((notinf = os_disconnect(user_name)) != 0){
      perror("KO os_disconnect");
      if(user_name) free(user_name);
      exit(EXIT_FAILURE);
    }
  }

  /*  Deallocazione memoria e chiusura lato client  */
  if(user_name) free(user_name);
  return 0;
}
