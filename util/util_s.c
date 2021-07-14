#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// Gestione di data e ora correnti
#include "retr_time.h"
// Costanti
#include "const.h"

extern char current_d[DATE_LEN+1];
extern char current_t[TIME_LEN+1];

// Elenco dei comandi disponibili lato server
void print_server_commands(){
    printf("Elenco dei comandi disponibili:\n");
    printf("$ help                  - mostra elenco comandi e significato\n");
    printf("$ showpeers             - mostra elenco peer connessi alla rete\n");
    printf("$ showneighbor [<peer>] - mostra i neighbor di un peer o la lista completa se peer viene omesso\n");
    printf("$ esc                   - termina il DS\n");
}
