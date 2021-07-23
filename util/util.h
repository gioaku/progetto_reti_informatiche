#ifndef UTILH
#define UTILH
#include <stdio.h>
#include "const.h"
#include <sys/stat.h>
#include <stdlib.h>

int valid_port(int);
int is_between(int, int, int);
int valid_date_s(char *);
int valid_date_i(int, int, int);
int in_time_interval(int, int, int, char *, char *);
int sooner_or_eq(int, int, int, int, int, int);
int sooner(int, int, int, int, int, int);
int file_exists(char *);
int file_exists_i(int, char, char *, int, int, int);
int file_name(char *, int, char, char *, int, int, int);
int create_path(char *);

#endif
