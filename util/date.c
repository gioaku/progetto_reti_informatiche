#include "date.h"

struct Date atod(char *buff)
{
    struct Date ret;
    if (sscanf(buff, DATE_FORMAT, &ret.y, &ret.m, &ret.d) != 3)
        printf("Error: date.c::atod fallita\n");
    return ret;
}

void dtoa(char *buff, struct Date d)
{
    printf("Debug: ret: %d\n", sprintf(buff, DATE_FORMAT, d.y, d.m, d.d));
    if (sprintf(buff, DATE_FORMAT, d.y, d.m, d.d) != 3)
        printf("Error: date.c::dtoa fallita da buff: %s a date: %d_%d_%d\n", buff, d.y, d.m, d.d);
    return;
}

int dvalid(struct Date d)
{
    if (d.y < MIN_YEAR || d.m < 1 || d.m > 12 || d.d < 1 || d.d > 31)
    {
        return 0;
    }
    if (d.m == 4 || d.m == 6 || d.m == 9 || d.m == 11)
        return (d.d <= 30);
    if (d.m == 2)
    {
        if (((d.y % 4 == 0) && (d.y % 100 != 0)) || (d.y % 400 == 0))
            return (d.d <= 29);
        else
            return (d.d <= 28);
    }
    return 1;
}

void dnext(struct Date *d)
{
    d->d++;
    if (dvalid(*d))
        return;
    d->d = 1;
    d->m++;
    if (dvalid(*d))
        return;
    d->m = 1;
    d->y++;
}

void dprev(struct Date *d){
    d->d--;
    if (dvalid(*d)){
        return;
    }
    d->d = 31;
    d->m--;
    if(d->m == 0){
        d->y--;
        d->m = 12;
        return;
    }
    while(!dvalid(*d)){
        d->d--;
    }
}

int sooner(struct Date f, struct Date s)
{
    return (f.y < s.y || (f.y == s.y && (f.m < s.m || (f.m == s.m && (f.d < s.d)))));
}

int soonereq(struct Date f, struct Date s)
{
    return (f.y < s.y || (f.y == s.y && (f.m < s.m || (f.m == s.m && (f.d <= s.d)))));
}

int dequal(struct Date f, struct Date s)
{
    return (f.y == s.y && f.m == s.m && f.d == s.d);
}

int update_date(struct Date *d)
{
    time_t now_time;
    struct tm *now_tm;

    now_time = time(NULL);
    now_tm = gmtime(&now_time);

    // se sono passate le 18 si considera la data di domani
    if (now_tm->tm_hour > 17)
    {
        now_tm->tm_mday++;
        mktime(now_tm);
    }

    if (!dequal(*d, (struct Date){now_tm->tm_mday, now_tm->tm_mon + 1, now_tm->tm_year + 1900})){
        *d = (struct Date){now_tm->tm_mday, now_tm->tm_mon + 1, now_tm->tm_year + 1900};
        return 1;
    }

    return 0;
}