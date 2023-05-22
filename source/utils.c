#include <util.h>

/*
 * funzione che concatena due stringhe se e solo se
 * la loro lunghezza sommata non supera il limite massimo [MAX_NAME]
 * */
char * valid_name(char * dirname , char * next){

    size_t len1,len2;

    len1 = strnlen(dirname , MAX_NAME);
    len2 = strnlen(next , MAX_NAME);

    char * str_ret = NULL;

    if(len1 + len2 + 1 > MAX_NAME)

        return NULL;

    str_ret = s_malloc(len1 + len2 + 2);

    strncpy( str_ret , dirname , len1);
    str_ret[len1] = '/';
    str_ret[len1 +1] = '\0';
    str_ret = strncat( str_ret , next , len1 + len2 + 2);

    return str_ret;

}

/*
 * safe malloc
 * se fallice termina il processo
 * */
void* s_malloc (unsigned long size) {

    void *elem = NULL;

    if ((elem = malloc(size)) == NULL && size != 0){

        perror("malloc :");
        exit(EXIT_FAILURE);

    }

    return elem;

}


/*
 * traduce stringhe in long positivi
 * */
long isNumber(const char* s) {

    char* e = NULL;
    errno = 0;

    long val = strtol(s, &e, 0);
    if(errno == ERANGE){

        //non e' rappresentabile
        return -1;

    }
    if (e != NULL && *e == (char)0)

        return val;

    return -1;

}


/*
 * funzione per leggere size bytes
 * anche se la write venisse interrotta da eventuali segnagli
 * */

size_t read_n(long fd, void *buf, size_t size) {
    size_t left = size;
    long r;
    char *bufptr = (char*)buf;

    while(left > 0) {
        if( (r=read((int)fd, bufptr, left)) == -1) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (r == 0)
            return 0;   // EOF
        left    -= r;
        bufptr  += r;
    }
    return size;
}


/*
 * funzione per scrivere size bytes
 * anche se la write venisse interrotta da eventuali segnagli
 * */
size_t write_n(int fd, void *buf, size_t size) {

    size_t left = size;
    long r;
    char * buf_ptr = (char*)buf;
    while(left>0) {

        if ((r=write((int)fd ,buf_ptr,left)) == -1) {

            perror("write_n");
            return -1;

        }
        if (r == 0)

            return 0;

        left -= r;
        buf_ptr += r;

    }
    return 1;
}