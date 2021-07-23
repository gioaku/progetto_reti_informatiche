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

    if (bind(sock->id, (struct sockaddr *)&(sock->addr), sizeof(sock->addr)) != 0)
        return -1;
    if (listen(sock->id, 2) != 0)
        return -1;

    return sock->id;
}

// Inizializzazione del socket connesso - ritorna socket id
int tcp_connect_init(int port)
{
    struct sockaddr_in addr;
    socklen_t addr_len;
    int sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    set_address(&(addr), (socklen_t *)&(addr_len), port);
    if (connect(sock, (struct sockaddr *)&(addr), sizeof(addr)) == 0)
        return -1;
    return sock;
}

// Accept della connessione tcp con i vicini - ritorna descrittore del nuovo socket - ritorna -1 in caso di errore
int accept_connection(int listener)
{
    struct sockaddr_in addr;
    socklen_t addr_len;

    addr_len = sizeof(addr);

    return accept(listener, (struct sockaddr *)&addr, &addr_len);
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
        printf("Messaggio %s ricevuto correttamente dal destinatario %d\n", buffer, port);
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

int handle_tcp_socket(int sock)
{
    char buffer[MAX_TCP_MSG + 1];
    char msg_type_buffer[MESS_TYPE_LEN + 1];
    int ret;

    while (1)
    {
        ret = recv(sock, (void *)buffer, MAX_TCP_MSG, 0);

        if (ret == 0)
            return 0;

        buffer[ret] = '\0';

        strncpy(msg_type_buffer, buffer, MESS_TYPE_LEN);
        msg_type_buffer[MESS_TYPE_LEN] = '\0';

        printf("TCP [%d] : Ricevuto messaggio %s\n", sock, msg_type_buffer);
        if (strcmp(msg_type_buffer, "ELAB_REQ"))
        {
            char type;
            int d, m, y;

            ret = sscanf(buffer, "%s %c %04d_%02d_%02d", msg_type_buffer, &type, &y, &m, &d);
            if (ret != 5)
                continue;
            
        }
    }
}