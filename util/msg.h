#ifndef MSG
#define MSG
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

#include "neighbors.h"

// Lunghezza header messaggio
#define HEADER_LEN 8
// Attesa dell'ack
#define USEC 15000
// Tentativi per inviare un messaggio
#define ACK_TRIES 10
// Stringa di localhost
#define LOCALHOST "127.0.0.1"
// Massima lunghezza messaggio udp
#define MAX_UDP_MSG 26
// Massima lunghezza messaggio tcp
#define MAX_TCP_MSG 21
// Tutte le porte per le receive
#define ALL_PORT -1

struct UdpSocket
{
    int id;
    struct sockaddr_in addr;
    socklen_t addr_len;
    char buffer[MAX_UDP_MSG];
};

struct TcpSocket
{
    int id;
    struct sockaddr_in addr;
    socklen_t addr_len;
    char buffer[MAX_TCP_MSG];
};

// Pulizia delle variabili per l'indirizzo del socket
void set_address(struct sockaddr_in *addr_p, socklen_t *len_p, int port);

// Inizializzazione del socket udp - restituisce il descrittore di socket
int udp_socket_init(struct UdpSocket *sock, int port);

// Inizializzazione del socket di ascolto tcp - ritorna socket id
int tcp_listener_init(struct TcpSocket * sock, int port);

// Inizializzazione del socket connesso - ritorna socket id - ritorna -1 altrimenti
int tcp_connect_init(int port);

// Accept della connessione tcp con i vicini - ritorna descrittore del nuovo socket - ritorna -1 in caso di errore
int accept_connection(int listener);

// Ricezione bloccante di un messaggio - ritorna la porta del mittente
int s_recv_udp(int socket, char *buffer, int buff_l);

// Invio di un messaggio - ritorna 1 se ha successo - ritorna 0 altrimenti
int s_send_udp(int socket, char *buffer, int buff_l, int send_port);

// Invio di un ack - ritorna 1 se ha successo - ritorna 0 altrimenti
int send_ack_udp(int socket, char *buffer, int send_port);

// Attesa e ricezione di un messaggio con header specifico - ritorna recv_port se arriva giusto - 0 altrimenti
int recv_udp(int socket, char *buffer, int buff_l, int port, char *correct_header);

// Invio di un messaggio fino all'arrivo dell' ack - ritorna 1 se ha successo - ritorna 0 altrimenti
int send_udp_wait_ack(int socket, char *buffer, int buff_l, int port, char *acked);

// Attesa del messagio con header specifico e invio dell'ack - ritorna recv_port se ack mandato con successo - ritorna 0 altrimenti
int recv_udp_and_ack(int socket, char *buffer, int buff_l, int port, char *correct_header, char *ack_type);

// Invio messaggio su socket tcp
void send_tcp(int sock, char *buffer, int msg_len);

// Ricezione messaggio su socket tcp
int recv_tcp(int sock, char *buffer);

#endif
