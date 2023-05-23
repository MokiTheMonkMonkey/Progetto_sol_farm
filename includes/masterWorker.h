#ifndef MASTER_H
#define MASTER_H

#include <util.h>
#include <threadsPool.h>
#include <signal.h>
#include <dirent.h>

extern sigset_t mask;


void cerca_File_Regolari( char * dirName );

char * ins_file_singoli( int argc , char * argv[] , int OptInd );

void *signalHandler();

int signalMask();

void masterExitFun();

void init_coda_con();

void set_standard_coda_con();

void master_connection();

#endif