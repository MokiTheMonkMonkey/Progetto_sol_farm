#include <collector.h>

/*
 * funzione che deallocare albero
 * */
void freeTree(TreeNode * cTree){

    if(!cTree){

        return;

    }
    if(cTree -> left){

        freeTree(cTree -> left);

    }
    if(cTree -> right){

        freeTree(cTree -> right);

    }
    free(cTree -> fileName);
    free(cTree);

}

/*
 * funzione di uscita che chiama la liberazione dell'albero
 * */
void collectorExitFun(){

    freeTree(B_S_Tree);

}


/*
 * creazione della socket dalla parte del collector
 * */
int sock_create(){

    int fd_sock , cfd;

    struct sockaddr_un sa;

    IS_MENO1(fd_sock = socket(AF_UNIX , SOCK_STREAM , 0 ) , "errore creazione socket :" , exit(EXIT_FAILURE) )


    memset(&sa, '\0' , sizeof(sa));

    strncpy( sa.sun_path , SOCK_NAME , SOCK_NAME_LEN );
    sa.sun_family = AF_UNIX;

    IS_MENO1 (bind(fd_sock , (struct sockaddr *) &sa , sizeof(sa)) , "errore bind :" , exit(EXIT_FAILURE) )

    IS_MENO1 (listen (fd_sock , SOMAXCONN) , "errore listen :", exit(EXIT_FAILURE) )

    IS_MENO1 (cfd = accept (fd_sock , 0 , NULL ) , "errore accept :" , exit(EXIT_FAILURE))

    return cfd;

}

/*
 * funzione per l'inserimento nell'albero binario di ricerca
 * */
void insTree(Mes nodoIns,TreeNode ** cTree) {

    //caso base l'albero e' vuoto
    if(*cTree == NULL){

        *cTree = s_malloc(sizeof(TreeNode));
        size_t nameSize = strnlen(nodoIns .nome ,MAX_NAME) + 1;
        (*cTree) -> fileName = s_malloc(nameSize);
        (*cTree) -> val = nodoIns . val;
        strncpy((*cTree) -> fileName, nodoIns . nome, nameSize);
        (*cTree) -> right = NULL;
        (*cTree) -> left = NULL;
        return;

    }
    //in caso il nuovo nodo sia minore, del corrente
    if((*cTree) -> val < nodoIns . val){

        insTree(nodoIns , &((*cTree) -> right));

    }
    else{
        //caso nuovo nodo >= nodo corrente

        insTree(nodoIns,&((*cTree) -> left));

    }


}


/*
 * funzione per stampare l'albero corrente
 * */
void printTree (TreeNode * cTree){

    if(!cTree){

        return;

    }
    printTree(cTree -> left);
    fprintf(stdout , "%ld %s\n" , cTree -> val , cTree -> fileName );
    printTree(cTree -> right);

}
