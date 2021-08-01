#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

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

struct Date get_start_date(struct Date today){

    FILE* fd;
    if (!file_exists(START_DATE_FILE))
    {
        create_path(START_DATE_DIR);
        fd = fopen(START_DATE_FILE, "w");
        fprintf(fd, DATE_FORMAT, today.y, today.m, today.d);
        fclose(fd);
        return today;
    }
    else
    {
        struct Date ret;
        fd = fopen(START_DATE_FILE, "r");
        fscanf(fd, DATE_FORMAT, &ret.y, &ret.m, &ret.d);
        fclose(fd);
        return ret;
    } 
}
