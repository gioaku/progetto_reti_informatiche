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
#include "msg.h"

// Pulizia delle variabili per l'indirizzo del socket
void set_address(struct sockaddr_in *addr_p, socklen_t *len_p, int port)
{
    memset(addr_p, 0, sizeof((*addr_p)));
    addr_p->sin_family = AF_INET;
    addr_p->sin_port = htons(port);
    inet_pton(AF_INET, LOCALHOST, &addr_p->sin_addr);
    (*len_p) = sizeof((*addr_p));
}

// Inizializzazione del socket udp - restituisce il descrittore di socket
int udp_socket_init(struct UdpSocket *sock, int port)
{
    sock->id = socket(AF_INET, SOCK_DGRAM, 0);
    set_address(&(sock->addr), (socklen_t *)&(sock->addr_len), port);

    if (bind(sock->id, (struct sockaddr *)&(sock->addr), sock->addr_len) != 0)
        return -1;

    return sock->id;
}

// Inizializzazione del socket di ascolto tcp - ritorna socked id
int tcp_listener_init(struct TcpSocket *sock, int port)
{
    sock->id = socket(AF_INET, SOCK_STREAM, 0);
    set_address(&(sock->addr), (socklen_t *)&(sock->addr_len), port);

    if (bind(sock->id, (struct sockaddr *)&(sock->addr), sock->addr_len) != 0)
        return -1;
    if (listen(sock->id, 2) != 0)
        return -1;

    return sock->id;
}

// Inizializzazione del socket connesso con un vicino - ritorna socket id
int tcp_connect_init(int port, struct TcpSocket *sock)
{
    sock->id = socket(AF_INET, SOCK_STREAM, 0);
    set_address(&(sock->addr), (socklen_t *)&(sock->addr_len), port);
    if (connect(sock->id, (struct sockaddr *)&(sock->addr), sock->addr_len))
        return -1;
    sock->port = port; 
    return sock->id;
}

// Accept della connessione tcp con i vicini - ritorna descrittore del nuovo socket - ritorna -1 in caso di errore
int accept_nb_connection(int listener, struct Neighbors nbs, struct TcpSocket *prev, struct TcpSocket *next)
{
    struct TcpSocket tmp;
    tmp.addr_len = sizeof(tmp.addr);

    if ((tmp.id = accept(listener, (struct sockaddr *)&tmp.addr, tmp.addr_len)) == -1)
    {
        return -1;
    }
    printf("ACCEPT addr.sin_port %d, htons(addr.sin_port) %d, ntohs(addr.sin_port) %d\n", tmp.addr.sin_port, htons(tmp.addr.sin_port), ntohs(tmp.addr.sin_port));
    tmp.port = ntohs(tmp.addr.sin_port);

    if (tmp.port == nbs.next)
    {
        printf("Accettata connessione tcp con il vicino successivo %d", tmp.port);
        next->port = tmp.port;
        next->id = tmp.id;
        next->addr = tmp.addr;
        next->addr_len = tmp.addr_len;
        return next->id;
    }
    if (tmp.port == nbs.prev)
    {
        printf("Accettata connessione tcp con il vicino successivo %d", tmp.port);
        prev->port = tmp.port;
        prev->id = tmp.id;
        prev->addr = tmp.addr;
        prev->addr_len = tmp.addr_len;
        return prev->id;
    }

    printf("Chiusa connessione con peer %d sconosciuto\n", tmp.port);
    close(tmp.id);
    return -1;
}

// Ricezione bloccante di un messaggio - ritorna la porta del mittente
int s_recv_udp(int socket, char *buffer, int buff_l)
{
    struct sockaddr_in send_addr;
    socklen_t send_addr_len;

    send_addr_len = sizeof(send_addr);

    recvfrom(socket, buffer, buff_l, 0, (struct sockaddr *)&send_addr, &send_addr_len);
    return ntohs(send_addr.sin_port);
}

// Invio di un messaggio - ritorna 1 se ha successo - ritorna 0 altrimenti
int s_send_udp(int socket, char *buffer, int buff_l, int send_port)
{
    struct sockaddr_in send_addr;
    socklen_t send_addr_len;

    send_addr_len = sizeof(send_addr);
    set_address(&send_addr, &send_addr_len, send_port);

    return sendto(socket, buffer, buff_l, 0, (struct sockaddr *)&send_addr, send_addr_len) > 0;
}

// Invio di un ack - ritorna 1 se ha successo - ritorna 0 altrimenti
int s_send_ack_udp(int socket, char *buffer, int send_port)
{
    if (s_send_udp(socket, buffer, MESS_TYPE_LEN + 1, send_port))
    {
        printf("Messaggio %s inviato correttamente al destinatario %d\n", buffer, send_port);
        return 1;
    }
    return 0;
}

// Attesa e ricezione di un messaggio con header specifico - ritorna 1 se arriva giusto - 0 altrimenti
int recv_udp(int socket, char *buffer, int buff_l, int port, char *correct_header)
{
    int recv_port;
    char temp_buffer[MESS_TYPE_LEN + 1];
    struct timeval util_tv;
    fd_set readset;

    util_tv.tv_sec = 0;
    util_tv.tv_usec = USEC;

    FD_ZERO(&readset);
    FD_SET(socket, &readset);

    // attesa messaggio
    select(socket + 1, &readset, NULL, NULL, &util_tv);

    // messaggio arrivato
    if (FD_ISSET(socket, &readset))
    {
        // lettura messaggio
        recv_port = s_recv_udp(socket, buffer, buff_l);

        sscanf(buffer, "%s", temp_buffer);
        temp_buffer[MESS_TYPE_LEN] = '\0';

        // se messaggio diverso scarta
        if (port != recv_port)
        {
            printf("Warning: [R] Arrivato un messaggio %s inatteso da [err] %d mentre attendevo %s da %d, scartato\n", temp_buffer, recv_port, correct_header, port);
        }
        else if (strcmp(correct_header, temp_buffer) != 0)
        {
            printf("Warning: [R] Arrivato un messaggio [err] %s inatteso da %d mentre attendevo %s da %d, scartato\n", temp_buffer, recv_port, correct_header, port);
        }

        // se messaggio giusto ritorna 1
        else
        {
            printf("Messaggio %s ricevuto correttamente dal mittente %d\n", temp_buffer, recv_port);
            return 1;
        }
    }
    return 0;
}

// Invio di un messaggio fino all'arrivo dell' ack - ritorna 1 se ha successo - ritorna 0 altrimenti
int send_udp_wait_ack(int socket, char *buffer, int buff_l, int port, char *acked)
{
    char recv_buffer[MAX_SOCKET_RECV];
    int tries = ACK_TRIES;

    // invia il messaggio
    if (!s_send_udp(socket, buffer, buff_l, port))
    {
        printf("Errore: [S] Impossibile inviare messaggio %s al destinatario %d\n", buffer, port);
    }
    printf("Messaggio %s inviato correttamente al destinatario %d\n", buffer, port);

    while (!recv_udp(socket, recv_buffer, MESS_TYPE_LEN, port, acked) && tries-- > 0)
        ;

    if (tries > -1)
    {
        printf("Messggio %s ricevuto correttamente dal destinatario %d\n", buffer, port);
        return 1;
    }

    printf("Errore: [R] impossibile ricevere messaggio %s dal destinatario %d\n", acked, port);
    return 0;
}

// Attesa del messagio con header specifico e invio dell'ack - ritorna 1 se ack mandato con successo - ritorna 0 altrimenti
int recv_udp_and_ack(int socket, char *buffer, int buff_l, int port, char *correct_header, char *ack_type)
{
    int tries = ACK_TRIES;

    // riceve messaggio
    while (!recv_udp(socket, buffer, buff_l, port, correct_header) && tries-- > 0)
        ;
    if (tries > -1)
    {
        // manda l'ack
        return s_send_ack_udp(socket, ack_type, port);
    }

    printf("Errore: [R] impossibile ricevere messaggio %s dal destinatario %d\n", correct_header, port);
    return 0;
}
