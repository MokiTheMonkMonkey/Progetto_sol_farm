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

                signExit = 1;
                pthread_exit(NULL);

            case SIGUSR1:

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

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;

    //ignoro il SIGPIPE
    if(sigaction(SIGPIPE,&sa, NULL) == -1) {
        perror("[MASTERWORKER] sigaction SIGPIPE");
        return -1;
    }

    sigemptyset(&mask);
    NOT_ZERO(sigaddset(&mask, SIGHUP),"sigaddset ")   // aggiunto SIGHUP alla maschera
    NOT_ZERO(sigaddset(&mask, SIGINT),"sigaddset ")   // aggiunto SIGINT alla maschera
    NOT_ZERO(sigaddset(&mask, SIGQUIT),"sigaddset ")  // aggiunto SIGQUIT alla maschera
    NOT_ZERO(sigaddset(&mask, SIGTERM),"sigaddset ")  // aggiunto SIGTERM alla maschera
    NOT_ZERO(sigaddset(&mask, SIGUSR1),"sigaddset ")  // aggiunto SIGUSR1 alla maschera
    //maschero i segnali
    if(pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) {

        perror("[MASTERWORKER] pthread_sigmask");
        return -1;

    }

    return 0;

}

/*
 * funzione di terminazione del master
 * */
void masterExitFun(){

    free(coda_concorrente.delay);
    free(coda_concorrente.workers);

    while(coda_concorrente.curr != 0){

        free(coda_concorrente.file_path[coda_concorrente.start]);
        coda_concorrente.start = (coda_concorrente.start + 1) % coda_concorrente.lim;
        coda_concorrente.curr--;

    }
    free(coda_concorrente.file_path);
    /*
    while(coda_concorrente.coda){

        next = coda_concorrente.coda -> next;
        free(coda_concorrente.coda -> nome);
        free(coda_concorrente.coda);
        coda_concorrente.coda = next;

    }
    */

}


/*
 * inizializzazione coda concorrente
 * */
void init_coda_con(){

    coda_concorrente.th_number = 0;
    coda_concorrente.delay = s_malloc(sizeof(struct timespec));
    coda_concorrente.delay -> tv_sec = 0;
    coda_concorrente.delay -> tv_nsec = 0;
    coda_concorrente.lim = 0;
    coda_concorrente.curr = 0;
    coda_concorrente.file_path = NULL;

}

/*
 * funzione che setta allo standard tutte le statistiche che non sono state settate
 * */
void set_standard_coda_con(){

    if(coda_concorrente.th_number == 0)

        coda_concorrente.th_number = 4;

    coda_concorrente.workers = s_malloc(sizeof(pthread_t) * coda_concorrente.th_number);
    terMes = (int)coda_concorrente.th_number;

    if(coda_concorrente.lim == 0)

        coda_concorrente.lim = 8;

    coda_concorrente.file_path = s_malloc(sizeof(char*) * coda_concorrente.lim);
    coda_concorrente.start = 0;
    coda_concorrente.last = 0;

}

/*
 * funzione che si connette al socket dal master
 * */

void master_connection(){

    int e;
    LOCK(&sock_mutex)
    IS_MENO1(fd_sock = socket (AF_UNIX , SOCK_STREAM , 0 ) , "errore creazione socket:" , exit(EXIT_FAILURE))

    struct sockaddr_un sa;

    //imposto il tempo di attesa per riprovare la connect 1 secondo
    struct timespec wait;

    wait.tv_nsec = 50000000;
    wait.tv_sec = 0;

    sa.sun_family = AF_UNIX;
    strncpy( sa.sun_path , SOCK_NAME , SOCK_NAME_LEN );
    sa.sun_path[SOCK_NAME_LEN] = '\0';

    //provo per 10 volte a connettere
    int i;
    for(i = 0; i < 10 ; i++){

        if((errno = 0) , connect(fd_sock , (struct sockaddr *)&sa , 108) == 0){

            break;

        }

        if(errno != ENOENT){

            e = errno;
            perror("connect ");
            exit(e);

        } else{

            if((errno = 0),nanosleep(&wait, NULL) == -1){

                e= errno;
                perror("nanosleep del sender :");
                exit(e);

            }

        }

    }
    if(i==10){

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

    //finche' la cartella non è vuota,si veirfica un errore o non viene mandato il segnale SIGINT
    while( ( errno = 0 ) , (( info = readdir(dir)) != NULL && !signExit) ){

        if( !( file_name = valid_name ( dirName , info -> d_name ) ) ){

            fprintf( stderr, "nome torppo lungo nella directory : %s" , dirName );

        }
        else if(stat(file_name , &d_stat) == -1){

            fprintf(stderr,"errore nella stat nel file :%s",file_name);
            free(file_name);
            exit(2);

        }
        else if((strncmp( info -> d_name , "." , 1) != 0) && (strncmp( info -> d_name , ".." , 2)) != 0){//controllo che non siano le cartelle "." o ".."

            //controllo se il file e' regolare
            if (S_ISREG(d_stat.st_mode)) {

                push_coda_con( file_name );

                if( nanosleep( coda_concorrente.delay , NULL ) != 0 ){

                    perror ("nanosleep :");
                    exit ( EXIT_FAILURE );

                }


            } else {

                if ( S_ISDIR ( d_stat.st_mode ) && !signExit) {

                    cerca_File_Regolari ( file_name );

                }
                else{

                    if(!signExit)

                        fprintf(stderr , "nella cartella %s torvato file non regolare :%s\n" , dirName , file_name);

                }


            }


        }

        free(file_name);

    }

    errno = 0;
    if(closedir( dir ) != 0 ) {

        perror("closedir :");
        exit(errno);

    }

}

/*
 * funzione che inserisce i file da argv
 * */
char * ins_file_singoli( int argc , char * argv[] , int OptInd ){


    struct stat c_stat;
    while( !signExit && OptInd < argc  ){

        if(strnlen(argv[OptInd],MAX_NAME) == MAX_NAME && argv[OptInd][MAX_NAME] != '\0'){

            fprintf( stderr , "il nome del file %s supera il limite di 255 caratteri :" , argv[OptInd++]);

        }

        if( (stat(argv[OptInd] , &c_stat) ) == -1 ){

            fprintf( stderr, "errore nel file :%s\n" , argv[OptInd++] );

        }
        else {
            if (S_ISREG(c_stat.st_mode) && !signExit) {

                push_coda_con(argv[OptInd++]);

                if (nanosleep(coda_concorrente.delay, NULL) != 0) {

                    perror("nanosleep : ");
                    exit(EXIT_FAILURE);

                }

            } else {

                fprintf(stderr, "errore nel file :%s\n", argv[OptInd++]);

            }

        }
    }

    return NULL;

}

