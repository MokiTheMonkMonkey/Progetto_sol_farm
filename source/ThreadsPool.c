#include <threadsPool.h>

/*
 * push della coda concorrente
 * */
void push_coda_con(char * filePath){

    LOCK(&coda_mutex)

    while(coda_concorrente.curr == coda_concorrente.lim && !signExit){

        //coda troppo piena aspetto che si svuoti
        WAIT(&full_coda_cond, &coda_mutex)

    }

    /*
     * è stato mandato un segnale che deve far terminare il programma
     * smetto di inserire in coda
     * */
    if(signExit){

        no_more_files = 1;
        BCAST(&void_coda_cond)
        UNLOCK(&coda_mutex)

        return;

    }

    //inserisco il nodo in coda
    size_t file_len = strnlen(filePath,MAX_NAME) + 1;
    coda_concorrente.file_path[coda_concorrente.last] = s_malloc(file_len);
    strncpy(coda_concorrente.file_path[coda_concorrente.last], filePath , file_len);
    coda_concorrente.curr++;
    coda_concorrente.last = (coda_concorrente.last + 1) % coda_concorrente.lim;

    //la coda non e' piu' vuota e mando il messaggio ai threads
    SIGNAL(&void_coda_cond);
    UNLOCK(&coda_mutex);

}

/*
 * pop della coda concorrente
 * */
char * pop_coda_con(){

    LOCK(&coda_mutex)

    //aspetto che la coda si riempia o che arrivi il messaggio di teminazione
    while(!coda_concorrente.curr && !no_more_files){

        WAIT(&void_coda_cond, &coda_mutex)

    }


    //se il messaggio di terminazione e' arrivato ritorno null
    if(no_more_files && !coda_concorrente.curr){

        //sono finiti i file
        BCAST(&void_coda_cond)
        UNLOCK(&coda_mutex)
        return NULL;

    }


    if(!(strncmp(coda_concorrente.file_path[coda_concorrente.start],"quit",4))){

        //e' il messaggio di terminazione quindi dealloco e avviso gli altri threads che la coda non verra' riempita
        free(coda_concorrente.file_path[coda_concorrente.start]);
        no_more_files = 1;
        coda_concorrente.curr--;
        BCAST(&void_coda_cond)
        UNLOCK(&coda_mutex)
         return NULL;

    }

    //prendo la stringa dalla coda
    char * ret = coda_concorrente.file_path[coda_concorrente.start];
    coda_concorrente.curr--;
    coda_concorrente.start = (coda_concorrente.start + 1) % coda_concorrente.lim;

    //avviso il master che la coda non e' piu' piena e rilascio la mutex
    SIGNAL(&full_coda_cond)
    UNLOCK(&coda_mutex)

    return ret;

}


/*
 * funzione worker che prende dalla coda concorrente e chiama la workerFun
 *
 * */
void * worker(){


    char* nomeFile;

    while(1) {

        //faccio la pop della coda concorrente
        if(!(nomeFile = pop_coda_con())){

            //la coda e' finita
            LOCK(&ter_mes_mutex)
            if(terMes > 1){

                //non e' l'ultimo thread quindi diminuisco terMes e termino il thread
                terMes--;

                UNLOCK(&ter_mes_mutex)

                return NULL;

            }else{

                //e' l'ultimo thread quindi mando il messaggio di terminazione e termino il thread
                UNLOCK(&ter_mes_mutex)

                send_message(NULL);

                return NULL;

            }

        }

        //calcolo il valore del file e lo mando
        worker_Fun(nomeFile);

    }


}
/*
 * funzione invocata dai threads che fa il calcolo del file
 * e manda i messaggi al socket
 * */
void worker_Fun(void* filepath){

    //dichiaro le variabili
    char * filePath = (char*) filepath;

    FILE * fd = NULL;
    long int i,lBuf;

    //apro il file
    if((fd = fopen (filePath , "r")) == NULL){

        fprintf(stderr,"ERRORE fopen nel file: %s\n",filePath);
        return;

    }

    Mes message;
    message.val = 0;
    i=0;
    //calcolo il valore
    while(fread (&lBuf , sizeof (long int) ,1,fd) != sizeof(long int) && !feof(fd)){

        message.val += lBuf * (i++);

    }
    //se si è interrotto per motivi diversi da EOF
    if(!feof (fd)){

        if(fclose (fd) == EOF){

            int err = errno;
            perror("fclose :");
            exit(err);

        }
        fprintf (stderr,"ERRORE thread worker fread nel file:%s\n",filePath);
        return;

    }

    //chiudo il file

    if((errno = 0),fclose (fd) == EOF){

        int err = errno;
        perror("fclose :");
        exit(err);

    }

    //mando il messaggio
    message.nome = filePath;

    send_message(&message);


}

/*
 * funzione per mandare messaggi sul socket
 * */
void send_message(Mes * to_send) {

    size_t w_bites  =0;
    int checkPrint;

    LOCK(&sock_mutex)

    //controllo che la connessione al socket sia avvennuta
    while(!is_set_sock){

        WAIT(&sock_cond, &sock_mutex)

    }
    do{
        checkPrint = printM;

        //controllo che non sia arrivata una richiesta di stampa dell'albero
        if(!checkPrint) {

            if (to_send) {

                w_bites = strnlen(to_send->nome, MAX_NAME) + 1;

                //mando la lunghezza della stringa
                IS_MENO1(write_n(fd_sock, &w_bites, sizeof(size_t)),"srittura numero bytes ", exit(EXIT_FAILURE))

                //mando la stringa
                IS_MENO1(write_n(fd_sock, to_send->nome, w_bites),"scrittura messaggio ", exit(EXIT_FAILURE))

                //mando il valore calcolato del file
                IS_MENO1(write_n(fd_sock, &(to_send->val), sizeof(long int)),"scrittura valore ", exit(EXIT_FAILURE))

                //dealloco la stringa
                free(to_send->nome);

            } else {

                //mando il messaggio di terminazione
                w_bites = -2;
                IS_MENO1(write_n(fd_sock, &w_bites, sizeof(size_t)),"write quit ", exit(EXIT_FAILURE))

                UNLOCK(&sock_mutex)

                return;


            }

        }
        else{

            //mando la richiesta di stampa
            w_bites =-3;
            IS_MENO1(write_n(fd_sock, &w_bites, sizeof(size_t)) , "write print request ", exit(EXIT_FAILURE))
            //resetto la variabile di stampa
            printM = 0;

        }

        //controllo che non sia arrivata una richiesta di stampa
    }while(checkPrint);

    UNLOCK(&sock_mutex)



}