#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int isn_between(int x, int a, int b)
{
    return !(a > b) ? (x > a || x < b) : (x > a && x < b);
}

int other(int x, int a, int b)
{
    return (b < x && a > x) || (b > a && (x < a || x > b));
}

int main()
{
    int i, j, k;
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            for (k = 0; k < 3; k++)
            {
                printf("differ: a : %d, b: %d, x: %d, isbet: %d, oth: %d\n", j, k , i, isn_between(i, j, k), other(i, j, k));
            }
        }
    }
    printf("Finished\n");
}