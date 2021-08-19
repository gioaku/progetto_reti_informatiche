#ifndef UTILH
#define UTILH
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>

#define BUFF_SIZE 100

// Controlla che port sia un intero positivo su due byte
int valid_port(int port);

// Ritorna 1 se il file esiste - ritorna 0 altrimenti 
int file_exists(char * file);

// Crea le cartelle necessarie a formare il path
int create_path(char * path);

#endif
