#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <util.h>


typedef struct treeNode{

    long val;
    char * fileName;
    struct treeNode * left;
    struct treeNode * right;

}TreeNode;

extern TreeNode * B_S_Tree;


void collectorExitFun();

void freeTree(TreeNode * cTree);

void printTree (TreeNode * cTree);

void insTree(Mes nodoIns,TreeNode ** cTree);

int sock_create();

#endif