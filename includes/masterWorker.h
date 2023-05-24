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

void master_connection();

#endif