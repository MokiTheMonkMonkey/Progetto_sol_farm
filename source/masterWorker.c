#include <masterWorker.h>

/*
 * signal handler
 * */
void *signalHandler(){

    int signum,err;

    while ( 1 ) {

        //aspetto i segnali
        err = sigwait(&mask, &signum);

        if(err != 0) {

            perror("sigwait ");        //sigwait smaschera e si sospende
            exit(EXIT_FAILURE);

        }

        //controllo qual' e' il segnale
        switch(signum) {

            case SIGHUP:
            case SIGINT:
            case SIGQUIT:
            case SIGTERM:
                //cambio il flag che impedisce l'inserimento di nuovi elementi in coda
                signExit = 1;
                pthread_exit(NULL);

            case SIGUSR1:
                //cambio il flag che manda una richiesta di stampa al collector
                printM =1;
                break;

            default:
                ;

        }

    }

}

/*
 * funzione che maschera i segnali
 * */
int signalMask(){

    //inizializzo sa e gli associo SIG_IGN
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;

    //ignoro il SIGPIPE
    IS_MENO1(sigaction(SIGPIPE,&sa, NULL),"sigaction SIGPIPE",exit(EXIT_FAILURE))

    //inizializzo la maschera e inserisco i segnali da mascherare
    sigemptyset(&mask);
    NOT_ZERO(sigaddset(&mask, SIGHUP),"sigaddset ")   // aggiunto SIGHUP alla maschera
    NOT_ZERO(sigaddset(&mask, SIGINT),"sigaddset ")   // aggiunto SIGINT alla maschera
    NOT_ZERO(sigaddset(&mask, SIGQUIT),"sigaddset ")  // aggiunto SIGQUIT alla maschera
    NOT_ZERO(sigaddset(&mask, SIGTERM),"sigaddset ")  // aggiunto SIGTERM alla maschera
    NOT_ZERO(sigaddset(&mask, SIGUSR1),"sigaddset ")  // aggiunto SIGUSR1 alla maschera
    //maschero i segnali
    NOT_ZERO(pthread_sigmask(SIG_BLOCK, &mask, NULL) ,"pthread_sigmask ")

    return 0;

}

/*
 * funzione di terminazione del master
 * dealloca tutti i possibili file rimanenti
 * */
void masterExitFun(){


    free(coda_concorrente.delay);
    if(coda_concorrente.workers) {

        free(coda_concorrente.workers);

    }
    while(coda_concorrente.curr != 0){

        free(coda_concorrente.file_path[coda_concorrente.start]);
        coda_concorrente.start = (coda_concorrente.start + 1) % coda_concorrente.lim;
        coda_concorrente.curr--;

    }
    if(coda_concorrente.file_path)

        free(coda_concorrente.file_path);


}


/*
 * funzione che si connette al socket dal master
 * */

void master_connection(){

    int e;
    LOCK(&sock_mutex)
    IS_MENO1(fd_sock = socket (AF_UNIX , SOCK_STREAM , 0 ) , "errore creazione socket:" , exit(EXIT_FAILURE))

    struct sockaddr_un sa;

    //imposto il tempo di attesa per riprovare la connect 1/5 s
    struct timespec wait;

    wait.tv_nsec = 20000000;
    wait.tv_sec = 0;

    sa.sun_family = AF_UNIX;
    strncpy( sa.sun_path , SOCK_NAME , SOCK_NAME_LEN );
    sa.sun_path[SOCK_NAME_LEN] = '\0';

    //provo per 10 volte a connettere
    int i;
    for(i = 0; i < 10 ; i++){

        if((errno = 0) , connect(fd_sock , (struct sockaddr *)&sa , 108) == 0){

            //la connessione e' andata in porto, termino il ciclo
            i = -1;
            break;

        }

        if((e = errno),e != ENOENT && e != ECONNREFUSED){

            fprintf(stderr,"%d\n",e);
            perror("connect ");
            exit(e);

        } else{

            //in caso il socket non sia ancora stato creato dal collector aspetto e riprovo
            if((errno = 0),nanosleep(&wait, NULL) == -1){

                e= errno;
                perror("nanosleep della connect ");
                exit(e);

            }

        }

    }
    //se alla fine dei 10 tentativi non sono riuscito a connettermi chiudo il programma
    if(i!=-1){

        fprintf(stderr,"connect fallita\n");
        exit(EXIT_FAILURE);

    }

    is_set_sock = 1;

    SIGNAL(&sock_cond)

    UNLOCK(&sock_mutex)

}


/*
 * ricerca ricorsivamente e inserisce file regolari dalla cartella
 * */
void cerca_File_Regolari( char * dirName ){

    if(dirName == NULL){

        return;

    }
    struct stat d_stat;
    DIR * dir = NULL;

    errno = 0;
    //apro la cartella
    if((dir = opendir(dirName)) == NULL || signExit ){


        if( errno != 0){

            fprintf(stderr,"errore opendir : %s\n",dirName);
            return;


        }

        if(signExit){

            if(closedir(dir) != 0){

                fprintf(stderr,"errore closedir : %s\n",dirName);

            }

            return;

        }


    }

    struct dirent * info;
    char * file_name = NULL;

    //finche' la cartella non è vuota,si veirfica un errore o non viene mandato un segnale di terminazione
    while( ( errno = 0 ) , (( info = readdir(dir)) != NULL && !signExit) ){

        //controllo la lunghezza e genero il nuovo nome
        if( !( file_name = valid_name ( dirName , info -> d_name ) ) ){

            fprintf( stderr, "nome torppo lungo nella directory : %s" , dirName );

        }
        else if(stat(file_name , &d_stat) == -1){

            fprintf(stderr,"errore nella stat nel file :%s ",file_name);
            free(file_name);
            exit(EXIT_FAILURE);

        }
        else if((strncmp( info -> d_name , "." , 1) != 0) && (strncmp( info -> d_name , ".." , 2)) != 0){//controllo che non siano le cartelle "." o ".."

            //controllo se il file e' regolare
            if (S_ISREG(d_stat.st_mode)) {

                push_coda_con( file_name );

                NOT_ZERO( nanosleep( coda_concorrente.delay , NULL ),"nanosleep ")


            } else {

                //controllo che sia una directory e che non sia arrivato il segnale di terminazione
                if ( S_ISDIR ( d_stat.st_mode ) && !signExit) {

                    //chiamo la funzione risorisvamente
                    cerca_File_Regolari ( file_name );

                }
                else{
                    //non e' una directory
                    if(!signExit)

                        fprintf(stderr , "nella cartella %s torvato file non regolare :%s\n" , dirName , file_name);

                }


            }


        }

        //libero il nome
        free(file_name);

    }

    errno = 0;
    NOT_ZERO(closedir( dir ),"closedir :")

}

/*
 * funzione che inserisce i file da argv
 * */
char * ins_file_singoli( int argc , char * argv[] , int OptInd ){


    struct stat c_stat;
    //inserisco i file in coda concorrente finche' non finisce argv o non arriva un segnale
    while( !signExit && OptInd < argc  ){

        //controllo che il nome del file non sia troppo lungo
        if(strnlen(argv[OptInd],MAX_NAME) == MAX_NAME && argv[OptInd][MAX_NAME] != '\0'){

            fprintf( stderr , "il nome del file %s supera il limite di 255 caratteri :" , argv[OptInd++]);

        }

        //chiamo la stat
        if( (stat(argv[OptInd] , &c_stat) ) == -1 ){

            fprintf( stderr, "errore nel file :%s\n" , argv[OptInd++] );

        }
        else {
            //controllo che sia regolare
            if (S_ISREG(c_stat.st_mode) && !signExit) {

                //e' regolare e lo metto in coda
                push_coda_con(argv[OptInd++]);

                //aspetto il delay richiesto
                NOT_ZERO(nanosleep(coda_concorrente.delay, NULL),"nanosleep ")

            } else {

                //non e' regolare
                fprintf(stderr, "errore nel file :%s\n", argv[OptInd++]);

            }

        }
    }

    return NULL;

}