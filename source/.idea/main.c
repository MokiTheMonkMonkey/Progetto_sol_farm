#include <util.h>

/* controlla se s non e' 0,stampa errore e termina*/
#define NOT_ZERO(s,m){ \
        if( ( s )  != 0 ){ \
            perror ( m ) ;  exit ( EXIT_FAILURE ) ;\
        }\
}
/* controlla -1; stampa errore e termina */
#define ec_meno1(s,m) \
 if ( (s) == -1 ) {perror(m); exit(EXIT_FAILURE);}
/* controlla NULL; stampa errore e termina (NULL) */
#define ec_null(s,m) \
 if((s)==NULL) {perror(m); exit(EXIT_FAILURE);}
/* controlla -1; stampa errore ed esegue c */
#define ec_meno1_c(s,m,c) \
 if((s)==-1) {perror(m); c;}
#define LOCK( l )      if ( pthread_mutex_lock ( l ) != 0 )        { \
    fprintf(stderr, "ERRORE FATALE lock\n");		    \
    pthread_exit((void*)EXIT_FAILURE);			    \
  }
#define UNLOCK(l)    if (pthread_mutex_unlock(l)!=0)      { \
  fprintf(stderr, "ERRORE FATALE unlock\n");		    \
  pthread_exit((void*)EXIT_FAILURE);				    \
  }
#define WAIT(c,l)    if (pthread_cond_wait(c,l)!=0)       { \
    fprintf(stderr, "ERRORE FATALE wait\n");		    \
    pthread_exit((void*)EXIT_FAILURE);				    \
}
#define SIGNAL(c)    if (pthread_cond_signal(c)!=0)       {	\
    fprintf(stderr, "ERRORE FATALE signal\n");			\
    pthread_exit((void*)EXIT_FAILURE);					\
  }
#define BCAST(c)     if (pthread_cond_broadcast(c)!=0)    {		\
    fprintf(stderr, "ERRORE FATALE broadcast\n");			\
    pthread_exit((void*)EXIT_FAILURE);						\
  }

#define MAX_NAME 256

//variabili globali
int cond_Workers = 0,cond_Master = 0;
CodaCon coda_concorrente;


pthread_mutex_t mes_list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t mes_list_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t coda_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t coda_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t insert_coda_cond = PTHREAD_COND_INITIALIZER;


int main (int argc , char* argv[]){

    if(argc < 2){

        perror("allora :");
        exit(1);

    }

    int threads_number = 4,delay = 0,option;

    coda_concorrente.lim = 4;
    coda_concorrente.curr = 0;
    coda_concorrente.coda = NULL;
    coda_concorrente.last = NULL;

    while((option = getopt(argc,argv,"d:t:n:q"))){

        switch(option) {

            case 't':

                delay = isNumber(*argv);

            case 'n':

                threads_number = isNumber(*argv);

            case 'q':

                coda_concorrente.lim = isNumber(*argv);

            case 'd':



        }
    }

    pthread_t ins_coda;

    NOT_ZERO(pthread_create(&ins_coda,NULL, genera,argv),"pthread create:")


    pthread_t worker_Threads[threads_number];

    for(int i = 0; i < threads_number ; i++){

        NOT_ZERO(pthread_create (&worker_Threads[i] , NULL , worker , argv[i]),"pthread create:")

    }
    for(int i = 0 ; i < threads_number ; i++){

        NOT_ZERO(pthread_join (worker_Threads[i] , NULL),"pthread join")

    }
    NOT_ZERO(pthread_join(ins_coda,NULL),"join:")

    printList (l_Proc_Ptr);

    return 0;

}