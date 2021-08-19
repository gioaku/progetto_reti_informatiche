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
#include <sys/wait.h>

// Funzioni generiche
#include "util.h"
// Gestione date
#include "date.h"
// Gestione peer graph
#include "peerlist.h"
// Gestione messaggi
#include "msg.h"

// Path del file che contiene la start date
#define START_DATE_FILE "./data/ds/start_date.txt"
// Path della directory che contiene la start date
#define START_DATE_DIR "./data/ds/"
// Massima lunghezza di un messaggio proveniente da stdin per il server
#define MAX_STDIN_S 18
// Massima lunghezza di un comando per il server
#define MAX_COMMAND_S 13

// Elenco dei comandi disponibili lato server
void print_server_commands();

// Restituisce la start date salvata se presente - altrimenti salva today e lo restituisce
struct Date get_start_date(struct Date today);

// Manda a tutti peer la data aggiornata
send_updated_date(struct Date *today, struct UdpSocket sock);