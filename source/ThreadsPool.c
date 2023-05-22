#include <threadsPool.h>


/*
 * funzione per inserire in coda
 * */
int insert_coda_con(char * nomeFile){


    LOCK(&coda_mutex)

    while(!is_set_coda_con && coda_concorrente.curr == coda_concorrente.lim && !signExit){

        //coda troppo piena aspetto che si svuoti
        WAIT(&coda_cond,&coda_mutex)

    }

    if(signExit){

        no_more_files = 1;
        SIGNAL(&coda_cond)
        UNLOCK(&coda_mutex)

        return 0;

    }

    if(!coda_concorrente.coda){


        //non ci sono nodi quindi inserisco il primo
        coda_concorrente.coda = s_malloc(sizeof(NodoCoda));
        size_t dim = strnlen ( nomeFile , MAX_NAME) + 1;
        coda_concorrente.coda -> nome = s_malloc(dim);
        strncpy(coda_concorrente.coda -> nome , nomeFile , dim);
        coda_concorrente.coda -> next = NULL;
        coda_concorrente.last = coda_concorrente.coda;

    }
    else{

        //ci sono nodi quindi appendo all'ultimo
        NodoCoda * nuovoNodo = NULL;

        nuovoNodo = s_malloc(sizeof(NodoCoda));
        size_t dim = strnlen ( nomeFile , MAX_NAME) + 1;
        nuovoNodo -> nome = s_malloc(dim);
        nuovoNodo -> next = NULL;
        strncpy((nuovoNodo) -> nome , nomeFile ,dim);
        coda_concorrente.last -> next = nuovoNodo;
        coda_concorrente.last = nuovoNodo;

    }
    //aggiorno la quantità di nodi presenti
    coda_concorrente.curr++;

    SIGNAL(&coda_cond)

    UNLOCK(&coda_mutex)

    return 0;

}



/*
 * pop della coda concorrente
 * */
char * pop_Coda_Con(){

    NodoCoda * coda_next = NULL;

    LOCK(&coda_mutex)


    //aspetto che la coda si riempia o che arrivi il messaggio di teminazione
    while(!coda_concorrente.curr && !no_more_files){

        WAIT(&coda_cond,&coda_mutex)

    }


    //se il messaggio di terminazione e' arrivato ritorno null
    if(no_more_files && !coda_concorrente.curr){


        BCAST(&coda_cond)
        UNLOCK(&coda_mutex)
        return NULL;

    }



    size_t dim = strnlen(coda_concorrente.coda -> nome, MAX_NAME) + 1;
    char * fileName = s_malloc(dim);

    if(--coda_concorrente.curr == 0){

        (coda_concorrente.last) = NULL;

    }
    else{

        coda_next = (coda_concorrente.coda) -> next;

    }

    strncpy(fileName,coda_concorrente.coda -> nome, dim);

    free((coda_concorrente.coda) -> nome);
    free(coda_concorrente.coda);

    (coda_concorrente.coda) = coda_next;

    if(!strncmp(fileName , "quit" , 4)){

        no_more_files = 1;
        BCAST( &coda_cond )
        UNLOCK(&coda_mutex)
        return fileName;

    }

    SIGNAL( &coda_cond )
    UNLOCK( &coda_mutex )

    return fileName;

}


/*
 * funzione worker che prende dalla coda concorrente e chiama la workerFun
 *
 * */
void * worker(){


    char* nomeFile;

    while(1) {

        if (((nomeFile = pop_Coda_Con ()) && !strncmp ( nomeFile , "quit" , 4)) ) {



            LOCK(&ter_mes_mutex)

            terMes--;

            UNLOCK(&ter_mes_mutex)

            free(nomeFile);

            return NULL;

        }

        if(!nomeFile || signExit){

            if(nomeFile) {

                free(nomeFile);
                nomeFile = NULL;
            }
            LOCK(&ter_mes_mutex)
            if(terMes > 1){

                terMes--;

                UNLOCK(&ter_mes_mutex)

                return NULL;

            }else{

                UNLOCK(&ter_mes_mutex)

                send_message(NULL);

                return NULL;

            }



        }

        worker_Fun(nomeFile);

    }


}
/*
 * funzione invocata dai threads che fa il calcolo del file
 * e inserisce in coda mesaggi
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
 * thread dedicato a scirvere messaggi sulla socket
 * */
void send_message(Mes * to_send) {

    size_t w_bites  =0;
    int checkPrint;
    LOCK(&sock_mutex)
    while(!is_set_sock){

        WAIT(&sock_cond, &sock_mutex)

    }
    do{
        checkPrint = printM;

        if(!checkPrint) {

            if (to_send) {

                w_bites = strnlen(to_send->nome, MAX_NAME) + 1;


                IS_MENO1(write_n(fd_sock, &w_bites, sizeof(size_t)),"srittura numero bytes ", exit(EXIT_FAILURE))


                IS_MENO1(write_n(fd_sock, to_send->nome, w_bites),"scrittura messaggio ", exit(EXIT_FAILURE))


                IS_MENO1(write_n(fd_sock, &(to_send->val), sizeof(long int)),"scrittura valore ", exit(EXIT_FAILURE))


                free(to_send->nome);

            } else {

                w_bites = -2;

                IS_MENO1(write_n(fd_sock, &w_bites, sizeof(size_t)),"write quit ", exit(EXIT_FAILURE))

                if ((errno = 0), close(fd_sock) == -1) {

                    perror("socket close ");
                    exit(EXIT_FAILURE);

                }

                UNLOCK(&sock_mutex)

                return;


            }

        }
        else{


            IS_MENO1(write_n(fd_sock, &w_bites, sizeof(size_t)) , "write print request ", exit(EXIT_FAILURE))

            printM = 0;

            w_bites =-3;


        }

    }while(checkPrint);

    UNLOCK(&sock_mutex)


}