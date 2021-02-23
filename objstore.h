#if !defined(OBJSTORE_H)
#define OBJSTORE_H

#define TRUE 0
#define FALSE 1

  int os_connect(char * name);  //Inizia la connessione all ojstore, registrando il cliente con il NAME dato
  int os_store(char * name, void * block, size_t len); //Richiede all'object store la memorizzazione dell'oggetto puntato da block, per una lunghezza len, con il nome name
  void *os_retrieve(char * name); //Recupera dall'object store l'oggetto precedentemente memorizzato sotto il nome name.
  int os_delete(char * name); //Cancellla l'oggetto di nome name precedentemente memorizzato.
  int os_disconnect();  //Chiude la connessione all ojstore.

#endif
