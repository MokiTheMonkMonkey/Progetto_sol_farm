#define _GNU_SOURCE

#include <threadsPool.h>
#include <collector.h>
#include <masterWorker.h>

#include <getopt.h>

//variabili globali
int terMes, no_more_files = 0,fd_sock,is_set_sock = 0;


//maschera per i segnali
sigset_t mask;

//variabili atomiche per il signal handler
volatile sig_atomic_t  signExit = 0,printM = 0;

//strutture dati : coda_concorrente , coda messaggi e albero binario di ricerca
CodaCon coda_concorrente;
TreeNode * B_S_Tree = NULL;

//mutex per : messaggio di terminazione, coda_messaggi e coda_concorrente
pthread_mutex_t ter_mes_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sock_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t coda_mutex = PTHREAD_MUTEX_INITIALIZER;

//condizioni per : lista messaggi e coda concorrente
pthread_cond_t sock_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t void_coda_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t full_coda_cond = PTHREAD_COND_INITIALIZER;



int main (int argc , char* argv[]){

    if(argc < 2){

        perror("inserire argomenti :");
        exit(1);

    }

    //vari flag per le opzioni
    char nCase = 0 , qCase = 0;
    int d_Case = 0, tCase = 0 , option;

    char * dir_name = NULL;

    //inizializzo la coda concorrente
    init_coda_con();

    opterr = 0;

    //controllo le opzioni
    while((option = getopt(argc,argv,":d:t:n:q:")) != -1){

        switch(option) {

            case 't':

                ISSET_CODA(tCase, "delay inseribile una sola volta\n")
                MU_ZERO((tCase = isNumber (optarg )) , "inserire un delay positivo")

                //converto il tCase in secondi
                coda_concorrente.delay -> tv_sec = tCase / 1000;
                //converto il tCase da millisecondi a nanosecondi
                coda_concorrente.delay -> tv_nsec = (tCase % 1000) * 1000000;
                tCase = 1;

                break;

            case 'n':

                ISSET_CODA(nCase , "numero di threads inseribile un sola volta\n")
                nCase = 1;
                MU_ZERO((coda_concorrente.th_number = isNumber(optarg)), "inserire un numero di thread maggiore di zero" )

                break;

            case 'q':

                ISSET_CODA(qCase, "limite coda concorrente inseribile una sola volta\n")
                qCase = 1;
                MU_ZERO((coda_concorrente.lim = isNumber(optarg)) , "inserire un limite queque maggiore di zero" )

                break;

            case 'd':

                ISSET_CODA(d_Case , "inseririe solo una cartella da analizzare\n" )
                d_Case = 1;
                size_t dirLen = strnlen(optarg,MAX_NAME) + 1;
                dir_name = s_malloc(dirLen);
                strncpy(dir_name,optarg,dirLen);

                break;

            case ':':
            case '?':
            default:

                fprintf(stderr,"inserie opzione valida\n");
                tutorial()
                exit( 1 );

        }

    }

    //non sono stati inseriti file o cartelle tra gli argomenti
    if(optind == argc && !d_Case){

        fprintf( stderr, "nessuna cartella o file da analizzare.\n" );
        tutorial()
        exit ( 1 );

    }

    int pid;

    //maschero i segnali per entrambi i processi
    signalMask();

    /*
     * creo il processo collector
     *
     */


    IS_MENO1(pid = fork() , "errore fork :" , exit (1 ) )

    if(pid == 0){

        /*
         * e' il figlio
         *collector
         * */


        //libero la memoria allocata nel vecchio processo
        free(dir_name);
        free(coda_concorrente.delay);

        atexit(&collectorExitFun);

        int r_sock;
        size_t r_bites = 0;
        Mes message;

        r_sock = sock_create();


        while(1) {


            //leggo la lunghezza della stringa da leggere o eventuali segnali
            IS_MENO1(read(r_sock, &r_bites, sizeof(size_t)),"ERRORE collector read :", exit(EXIT_FAILURE))

            //messaggio di terminazione
            if (r_bites == -2)

                break;

            //richiesta di stampa
            if(r_bites == -3){

                printTree(B_S_Tree);

            }
            else {


                if (r_bites <= 0)

                    return -1;

                //alloco spazio per il nome del file
                message.nome = s_malloc(r_bites);

                IS_MENO1(read_n(r_sock, message.nome, r_bites),"ERRORE collector read :",exit(EXIT_FAILURE))

                if (!strncmp(message.nome, "quit", 4)) {


                    break;

                }

                IS_MENO1(read_n(r_sock, &(message.val), sizeof(long int)),"ERRORE collector read :",exit(EXIT_FAILURE))


                insTree(message, &B_S_Tree);
                free(message.nome);

            }

        }
        IS_MENO1(close(r_sock) , "close :" , return -1)

        printTree(B_S_Tree);

        return 0;

    }

    /*
     * processo padre
     * master Worker
     * */


    pthread_t signalH;

    atexit(&masterExitFun);

    //inizializzo ai valori standard le opzioni non scelte
    set_standard_coda_con();

    NOT_ZERO(pthread_create(&signalH,NULL, signalHandler,NULL),"errore signal handler ")

    int e = 0;

    /*
     * faccio partire i threads prima di mettere in coda i file singoli
     * cosi' posso far farli concorrere durante l'inserimento
     * */

    for(int i = 0; i < coda_concorrente.th_number; i++) {


        NOT_ZERO(pthread_create(&(coda_concorrente.workers[i]), NULL, worker, NULL ) , "errore creazione worker")

    }

    master_connection();

    if(optind != argc){

        ins_file_singoli( argc , argv , optind );

    }

    //è stata analizzata una cartella e controllo se non ci sono stati errori
    if(d_Case){

        cerca_File_Regolari ( dir_name );
        free(dir_name);

    }

    push_coda_con("quit");

    for(int i = 0; i < coda_concorrente.th_number ; i++){

        if(pthread_join( coda_concorrente.workers[i] , NULL ) != 0){

            //gestione errori e free
            perror("join worker ");
            return -1;

        }


    }

    if(waitpid(pid, NULL , 0) == -1){

        perror("errore nella WAIT PID :");
        exit(e);

    }

    if(!signExit) {

        if ((e = pthread_kill(signalH, SIGQUIT)) != 0) {

            perror("kill signal handler ");
            exit(e);

        }

        if (pthread_join(signalH, NULL) != 0) {

            perror("join signal handler ");
            exit(EXIT_FAILURE);

        }

    }

    return 0;

}
