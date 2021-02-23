#if !defined(OBJLIST_H)
#define OBJLIST_H

struct elemento {
    struct elemento * next;
    char * user_name;
};

typedef struct elemento ElementoLista;
typedef ElementoLista * ListaElementi;

void Delete_Connection(ListaElementi * l, char * user_name);
void Delete_List(ListaElementi * l);
int  Is_Already_Conn(ListaElementi l, char * user_name);
int Number_Elem(ListaElementi l);
void Print_List(ListaElementi l);
void Set_Connection(ListaElementi * l, char * user_name);

#endif
