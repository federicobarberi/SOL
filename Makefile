#MAKEFILE PER IL PROGETTO
CC	=	gcc
CFLAGS	+=	-std=c99	-Wall	-Werror	-g
LIBS	=	-lpthread

TARGETS	=	server	\
			client

.PHONY	:	all	clean	cleandata	cleanall

%.o	:	%.c
	$(CC)	$(CFLAGS)	-c	-o	$@	$<

%	:	%.c
	$(CC)	$(CFLAGS)	-o	$@	$<	$(LIBS)

test :	client	server
	./testsum.sh

client	:	libobj.a	objstore.o
	$(CC)	$(CFLAGS)	client.c	-o	client	-L.	-lobj	$(LIBS)

server 	:	libobj.a	objstore.o	liblist.a	objlist.o
	$(CC)	$(CFLAGS)	server.c	-o	server 	-L.	-lobj	-llist	$(LIBS)

objstore.o	:	objstore.c	objstore.h
	$(CC)	$(CFLAGS)	objstore.c	-c	-o	objstore.o

objlist.o	:	objlist.c	objlist.h
	$(CC)	$(CFLAGS)	objlist.c	-c	-o	objlist.o

libobj.a	:	objstore.o	objstore.h
	ar	rvs	libobj.a	objstore.o

liblist.a	:	objlist.o	objlist.h
	ar	rvs	liblist.a	objlist.o

all	:	$(TARGETS)

clean	:
	rm	-f	$(TARGETS)

cleandata	:
	rm	-rf	./data/*

cleanall	:	clean
	\rm -f *.o *~ *.a ./objstore.sock
