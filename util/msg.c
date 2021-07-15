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

// Costanti
#include "const.h"

// Pulizia delle variabili per l'indirizzo del socket
void set_address(struct sockaddr_in *addr_p, socklen_t *len_p, int port)
{
    memset(addr_p, 0, sizeof((*addr_p)));
    addr_p->sin_family = AF_INET;
    addr_p->sin_port = htons(port);
    inet_pton(AF_INET, LOCALHOST, &addr_p->sin_addr);
    (*len_p) = sizeof((*addr_p));
}

// Inizializzazione del socket UDP - restituisce il descrittore di socket
int udp_socket_init(struct sockaddr_in *addr_p, socklen_t *len_p, int port)
{
    int sock;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    set_address(addr_p, len_p, port);

    if (bind(sock, (struct sockaddr *)addr_p, (*len_p)) != 0)
    {
        perror("Error while binding");
        exit(0);
    }

    return sock;
}

// Ricezione bloccante di un messaggio - ritorna la porta del mittente
int s_recv_udp(int socket, char *buffer, int buff_l)
{
    struct sockaddr_in send_addr;
    socklen_t send_addr_len;

    send_addr_len = sizeof(send_addr);

    recvfrom(socket, buffer, buff_l, 0, (struct sockaddr *)&send_addr, &send_addr_len);
    printf("receive port %d tradotta in %d", send_addr.sin_port, ntohs(send_addr.sin_port));
    return ntohs(send_addr.sin_port);
}

// Invio di un messaggio - ritorna 1 se ha successo - ritorna 0 altrimenti
int s_send_udp(int socket, char *buffer, int buff_l, int send_port)
{
    struct sockaddr_in send_addr;
    socklen_t send_addr_len;

    send_addr_len = sizeof(send_addr);
    set_address(&send_addr, &send_addr_len, send_port);

    return sendto(socket, buffer, MESS_TYPE_LEN + 1, 0, (struct sockaddr *)&send_addr, send_addr_len) > 0;
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

// Attesa e ricezione di un messaggio da confrontare con messaggio specifico - ritorna 1 se arriva giusto - 0 altrimenti
int recv_udp(int socket, char *buffer, int buff_l, int port, char *correct_header)
{
    int recv_port;
    char temp_buffer[MESS_TYPE_LEN];
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
        temp_buffer[buff_l] = '\0';

        // se messaggio giusto ritorna
        if (port == recv_port && strcmp(correct_header, temp_buffer) == 0)
        {
            printf("Messaggio %s ricevuto correttamente dal mittente %d\n", temp_buffer, recv_port);
            return 1;
        }
        // altrimenti scarta
        else
        {
            printf("Warning: [R] Arrivato un messaggio %s inatteso da %d mentre attendevo %s da %d, scartato\n", temp_buffer, recv_port, correct_header, port);
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
    {
    }

    if (tries > -1)
    {
        printf("Messggio %s ricevuto correttamente dal destinatario %d\n", buffer, port);
        return 1;
    }

    printf("Errore: [R] impossibile ricevere messaggio %s dal destinatario %d\n", acked, port);
    return 0;
}

// Attesa del messagio specifico e invio dell'ack - ritorna 1 se ack mandato con successo - ritorna 0 altrimenti
int recv_udp_and_ack(int socket, char *buffer, int buff_l, int port, char *correct_header, char *ack_type)
{
    int tries = ACK_TRIES;

    // riceve messaggio
    while (!recv_udp(socket, buffer, buff_l, port, correct_header) && tries-- > 0)
    {
    }
    if (tries > -1){
        // manda l'ack
        return s_send_ack_udp(socket, ack_type, port);
    }
    
    printf("Errore: [R] impossibile ricevere messaggio %s dal destinatario %d\n", correct_header, port);
    return 0;
}
