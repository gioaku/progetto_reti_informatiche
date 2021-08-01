#ifndef DATEH
#define DATEH

#include "stdio.h"
#include "time.h"

// Lunghezza di una stringa di data
#define DATE_LEN 10
// Formato di una stringa di data
#define DATE_FORMAT "%04d_%02d_%02d"
// Lunghezza di una stringa di orario
#define TIME_LEN 8
// Formato di una stringa di orario
#define TIME_FORMAT "%02d:%02d:%02d"
// Lunghezza di una stringa di data in input
#define DATE_IN_LEN 10
// Formato di una stringa di data
#define DATE_IN_FORMAT "%02d:%02d:%04d"
// Minimo anno per una data della get
#define MIN_YEAR 2021

// Collect day, month and year
struct Date
{
    int d;
    int m;
    int y;
};

// String to date
struct Date atod(char *buff, char* format);

// Date to string
void dtoa(char *buff, struct Date d, char* format);

// Check date
int dvalid(struct Date d);

// Increase date by 1 day
void dnext(struct Date *d);

// Decrease date by 1 day
void dprev(struct Date *d);

// Check first date smaller than second
int sooner(struct Date f, struct Date s);

// Check first date smaller or equalt to second
int soonereq(struct Date f, struct Date s);

// Check same date
int dequal(struct Date f, struct Date s);

// Set d = today - return 1 if d changed
int update_date(struct Date *d);

#endif