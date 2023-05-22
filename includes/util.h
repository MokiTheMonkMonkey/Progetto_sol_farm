#if  !defined(UTIL_H)
#define UTIL_H
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

#define SOCK_NAME "farm.sck"
#define SOCK_NAME_LEN 9
#define MAX_NAME 256

/* controlla se s non e' 0,stampa errore e termina*/
#define NOT_ZERO(s,m){ \
        if( ( s )  != 0 ){ \
            perror ( m ) ;  exit ( EXIT_FAILURE ) ;\
        }\
}
#define tutorial(){ \
        fprintf(stderr,"istruzioni per l'uso del programma farm :\n\n   *inserire una lista di file regolari da analizzare come argomento;\n\n");\
        fprintf(stderr,"   *opzioni disponibili (selezionabili una sola volta):\n"); \
        fprintf(stderr,"        -n : permette di specificare il limite massimo (positivo) di threads\n");      \
        fprintf(stderr,"        -d : permette di specificare una cartella da analizzare\n");      \
        fprintf(stderr,"        -q : permette di specificare la lunghezza massima (positiva) della coda concorrente\n"); \
        fprintf(stderr,"        -t : permette di specificare il delay (maggiore o uguale a zero) tra le richieste ai threads\n"); \
}
#define MU_ZERO(s,m) \
 if ( (s) <= 0 ) {fprintf(stderr,m); \
 tutorial()   \
 exit(EXIT_FAILURE); \
 }

#define IS_MENO1(s,m,c) \
    if((s)==-1) {          \
    perror(m);             \
    c;                     \
 }
#define LOCK(l)      if (pthread_mutex_lock(l)!=0)        { \
    fprintf(stderr, "Errore lock\n");		    \
    pthread_exit((void*)EXIT_FAILURE);			    \
  }
#define UNLOCK(l)    if (pthread_mutex_unlock(l)!=0)      { \
  fprintf(stderr, "Errore unlock\n");		    \
  pthread_exit((void*)EXIT_FAILURE);				    \
  }
#define WAIT(c,l)    if (pthread_cond_wait(c,l)!=0)       { \
    fprintf(stderr, "Errore wait\n");		    \
    pthread_exit((void*)EXIT_FAILURE);				    \
}
#define SIGNAL(c)    if (pthread_cond_signal(c)!=0)       {	\
    fprintf(stderr, "Errore signal\n");			\
    pthread_exit((void*)EXIT_FAILURE);					\
  }
#define BCAST(c)     if (pthread_cond_broadcast(c)!=0)    {		\
    fprintf(stderr, "Errore broadcast\n");			\
    pthread_exit((void*)EXIT_FAILURE);						\
  }
#define ISSET_CODA(s,m) \
    if ( (s) != 0 )      { \
        fprintf(stderr, m); \
        tutorial()  \
        return -1;      \
    }

typedef struct mes {

    char * nome;
    long val;

}Mes;


char * valid_name (char * dirname , char * next);

void* s_malloc (unsigned long size);

long isNumber (const char* s);

size_t read_n (long fd, void *buf, size_t size);

size_t write_n (int fd, void *buf, size_t size);

#endif
