#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// Costanti
#include "util_s.h"

// Elenco dei comandi disponibili lato server
void print_server_commands()
{
    printf("Elenco dei comandi disponibili:\n");
    printf("$ help                  - mostra elenco comandi e significato\n");
    printf("$ showpeers             - mostra elenco peer connessi alla rete\n");
    printf("$ showneighbor [<peer>] - mostra i neighbor di un peer o la lista completa se peer viene omesso\n");
    printf("$ esc                   - termina il DS\n");
}

// Controlla se sono passate le 18 e in caso aggiorna buff con la data corrente e ritorna 1 - altrimenti non ha effetto e ritorna 0
int update_date(char *buff)
{
    time_t now_time;
    struct tm *now_tm;
    char current_d[DATE_LEN + 1];
    char current_t[TIME_LEN + 1];
    int ret;

    ret = 0;
    now_time = time(NULL);
    now_tm = gmtime(&now_time);

    sprintf(current_d, DATE_FORMAT, now_tm->tm_year + 1900, now_tm->tm_mon + 1, now_tm->tm_mday);
    sprintf(current_t, TIME_FORMAT, now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec);

    // data e ora effettiva di oggi
    current_d[DATE_LEN] = '\0';
    current_t[DATE_LEN] = '\0';

    // se le date coincidono e sono passate le 18 ritorna la data di domani e segnala la modifica
    if (strcmp("18:00", current_t) <= 0)
    {
        if (strcmp(buff, current_d) == 0)
            ret = 1;
        now_tm->tm_mday ++;
        mktime(now_tm);
        sprintf(buff, DATE_FORMAT, now_tm->tm_year + 1900, now_tm->tm_mon + 1, now_tm->tm_mday);
        buff[DATE_LEN] = '\0';
    }
    else
    {
        strncpy(buff, current_d, DATE_LEN + 1);
    }
    return ret;
}