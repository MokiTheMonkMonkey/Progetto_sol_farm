#ifndef THREADS_POOL
#define THREADS_POOL
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <pthread.h>
#include <sys/types.h>
#include <values.h>

#include <util.h>

extern volatile __sig_atomic_t signExit, printM;

typedef struct codaCon{//coda concorrente

    //delay espresso in millisecondi
    struct timespec * delay;
    //limite coda
    long lim;
    //quantita' di file presenti in coda
    long curr;
    //inizio coda
    long start;
    //fine coda
    long last;
    //numero di thread massimi
    long th_number;
    //threads
    pthread_t *workers;
    //coda limitata (rappresentata con un vettore di stringhe)
    char ** file_path;

}CodaCon;

//variabili condivise

extern int terMes,is_set_sock,no_more_files,fd_sock;


//coda concorrente e lista di messaggi
extern CodaCon coda_concorrente;

//mutex
extern pthread_mutex_t ter_mes_mutex;
extern pthread_mutex_t sock_mutex;
extern pthread_mutex_t coda_mutex;

//cond per le varie wait
extern pthread_cond_t sock_cond;
extern pthread_cond_t void_coda_cond;
extern pthread_cond_t full_coda_cond;


void * worker();

void worker_Fun(void* filepath);

void master_connection();

void send_message(Mes * to_send);

void push_coda_con(char * filePath);

char * pop_coda_con();

#endif