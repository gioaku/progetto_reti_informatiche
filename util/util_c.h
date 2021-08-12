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

// Funzioni generiche
#include "util.h"
// Gesitione date
#include "date.h"
// Gesione vicini
#include "neighbors.h"
// Gestion messaggi
#include "msg.h"

// Directory de registri
#define REGISTERS "data"
// Directory entries
#define ENTRIES "entries"
// Directory agglomerati
#define ELABS "elabs"
// Formato del path alle directory dei file dei registri
#define PATH_FORMAT "./%s/%04d/%c/%s/"
// Formato del path ai file dei registri
#define FILE_FORMAT "./%s/%04d/%c/%s/%04d_%02d_%02d.txt"
// Formato del nome dei file dei registri
#define FILENAME_FORMAT "%04d_%02d_%02d.txt"
// Massima lunghezza di un filename per le entries
#define FILENAME_LEN 14
// Massima lunghezza di un path
#define MAX_PATH_LEN 22
// Massima lunghezza di un file
#define MAX_FILE_LEN 36
// Massima lunghezza di un messaggio proveniente da stdin per il peer
#define MAX_STDIN_C 40
// Massima lunghezza di un comando per il peer
#define MAX_COMMAND_C 6

// Elenco dei comandi disponibili lato client
void print_client_commands();

// Ulteriore ausilio per comprendere i comandi
void help_client(int i);

// Scrive la stringa del path dai dati forniti
int get_path_string(char *path, int port, char type, char *dir);

// Scrive la stringa del file dai dati forniti
int get_file_string(char *file, int port, char type, char *dir, struct Date date);

// Scrive la stringa del nome del file dai dati forniti
int get_filename_string(char *filename, struct Date date);

// Aggiunge l'entry qty nel file relativo a date e type creandolo se non trovato
void append_entry(int port, struct Date date, char type, int qty);

// Apre un registro in in modalia' mode
FILE* open_reg(int port, char type, struct Date date, char* mode);

// Controlla che il periodo fornito sia valido e inizializza from e to
int check_period(char * period, struct Date start_date, struct Date today, struct Date *from, struct Date *to);

// Ottiene il dato aggregato e se necessario lo salva - ritorna -1 altimenti
int get_total(int udp, int port,char type, struct Date date, struct Neighbors nbs);

// Restituisce totale di tipo type in data d/m/y - ritorna -1 altrimenti
int get_saved_elab(int port, char type, struct Date date);

// Restituisce la somma delle entry del file - ritorna 0 se file non trovato
int get_entries_sum(int port, char type, struct Date date);

// Crea il file aggregato con la quantita' qty
void create_elab(int port, char type, struct Date date, int qty);

// Gestisce una richiesta di connessione tcp
void handle_tcp_socket(int port, int sock);

// Raccoglie le entries(type, date) dei peer di tutta la rete
int collect_all_entries(int port, int udp, char type, struct Date date);

