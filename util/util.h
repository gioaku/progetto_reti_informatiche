#ifndef UTILH
#define UTILH
#include <stdio.h>
#include "const.h"
#include <sys/stat.h>
#include <stdlib.h>

int valid_port(int);
int is_between(int, int, int);
int valid_date_i(int, int, int);
int file_exists(char *);
int create_path(char *);
int sooner_or_eq(int d1, int m1, int y1, int d2, int m2, int y2);
int sooner(int d1, int m1, int y1, int d2, int m2, int y2);

#endif