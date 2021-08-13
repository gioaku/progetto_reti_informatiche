#ifndef UTILH
#define UTILH
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>

#define BUFF_SIZE 100

// check port is a positive integer on two bytes
int valid_port(int port);

int file_exists(char * file);
int create_path(char * path);
void pdebug(char *str);
#endif
