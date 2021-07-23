#include "util.h"

int valid_port(int port)
{
    return (port >= 0 && port <= 65536);
}

int is_between(int x, int a, int b)
{
    return (a > b) ? (x > a || x < b) : (x > a && x < b);
}

int valid_data(char *buff)
{
    int d, m, y;
    if (sscanf(buff, "%d:%d:%d", &y, &m, &d) != 3)
    
    {
        printf("sscanf diverso da 3");
        return 0;
    }

    if (y < MIN_YEAR || m < 1 || m > 12 || d < 1 || d > 31)
    {
        printf("y : %d, m : %d, d : %d  non validi", y, m, d);
        return 0;
    }

    return 1;
}