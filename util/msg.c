#include "msg.h"

void set_address(struct sockaddr_in *addr_p, socklen_t *len_p, int port)
{
    memset(addr_p, 0, sizeof((*addr_p)));
    addr_p->sin_family = AF_INET;
    addr_p->sin_port = htons(port);
    inet_pton(AF_INET, LOCALHOST, &addr_p->sin_addr);
    (*len_p) = sizeof((*addr_p));
}

int udp_socket_init(struct UdpSocket *sock, int port)
{
    sock->id = socket(AF_INET, SOCK_DGRAM, 0);
    set_address(&(sock->addr), (socklen_t *)&(sock->addr_len), port);

    if (bind(sock->id, (struct sockaddr *)&(sock->addr), sock->addr_len) != 0)
        return -1;

    return sock->id;
}

int tcp_listener_init(struct TcpSocket *sock, int port)
{
    int ret;
    int reuse;
    sock->id = socket(AF_INET, SOCK_STREAM, 0);

    if (sock->id == -1)
    {
        return -1;
    }

    set_address(&(sock->addr), (socklen_t *)&(sock->addr_len), port);

    reuse = 1;
    // Per evitare che il socket rimanga occupato quando il programma viene forzato a uscire
    if (setsockopt(sock->id, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
        perror("Errore: <tcp_listener_init> setsockopt(SO_REUSEADDR) fallita");

    if ((ret = bind(sock->id, (struct sockaddr *)&(sock->addr), sizeof(sock->addr))) != 0)
    {
        printf("Errore: <tcp_listener_init> errore durante il binding ret: %d\n", ret);
        return -1;
    }
    if ((ret = listen(sock->id, 2)) != 0)
    {
        printf("Errore: <tcp_listener_init> errore durante il listening ret: %d\n", ret);
        return -1;
    }

    return sock->id;
}

int tcp_connect_init(int port)
{
    struct sockaddr_in addr;
    socklen_t addr_len;
    int sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    set_address(&(addr), (socklen_t *)&(addr_len), port);
    if (!connect(sock, (struct sockaddr *)&(addr), sizeof(addr)) == 0)
    {
        printf("Errore: <tcp_connect> impossibile connettersi al listner di %d\n", port);
        return -1;
    }
    printf("TCP: connessione stabilita con peer %d sul socket %d\n", port, sock);
    return sock;
}

int accept_connection(int listener)
{
    struct sockaddr_in addr;
    socklen_t addr_len;
    int ret;

    addr_len = sizeof(addr);
    if ((ret = accept(listener, (struct sockaddr *)&addr, &addr_len)) == -1)
    {
        printf("Errore: <accept_connection> impossibile accettare connessione sul listener\n");
    }
    else
    {
        printf("TCP: richiesta di connessione accettata sul socket %d\n", ret);
    }
    return ret;
}

int s_recv_udp(int socket, char *buffer, int buff_l)
{
    struct sockaddr_in send_addr;
    socklen_t send_addr_len;

    send_addr_len = sizeof(send_addr);

    recvfrom(socket, buffer, buff_l, 0, (struct sockaddr *)&send_addr, &send_addr_len);
    return ntohs(send_addr.sin_port);
}

int s_send_udp(int socket, char *buffer, int buff_l, int send_port)
{
    struct sockaddr_in send_addr;
    socklen_t send_addr_len;

    send_addr_len = sizeof(send_addr);
    set_address(&send_addr, &send_addr_len, send_port);

    return sendto(socket, buffer, buff_l, 0, (struct sockaddr *)&send_addr, send_addr_len) > 0;
}

int send_ack_udp(int socket, char *buffer, int send_port)
{
    if (s_send_udp(socket, buffer, HEADER_LEN, send_port))
    {
        printf("UDP: inviato messaggio '%s' al destinatario %d\n", buffer, send_port);
        return 1;
    }
    printf("Errore: [S] Impossibile inviare ack %s al destinatario %d\n", buffer, send_port);
    return 0;
}

int recv_udp(int socket, char *buffer, int buff_l, int port, char *correct_header)
{
    int recv_port;
    char header_buff[HEADER_LEN + 1];
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
        buffer[buff_l] = '\0';
        sscanf(buffer, "%s", header_buff);
        header_buff[HEADER_LEN] = '\0';

        // se messaggio diverso scarta
        if (port != recv_port && port != ALL_PORT)
        {
            printf("Warning: [R] Arrivato un messaggio %s inatteso da %d [err] mentre attendevo %s da %d, scartato\n", header_buff, recv_port, correct_header, port);
        }
        else if (strcmp(correct_header, header_buff) != 0)
        {
            printf("Warning: [R] Arrivato un messaggio %s [err] inatteso da %d mentre attendevo %s da %d, scartato\n", header_buff, recv_port, correct_header, port);
        }

        // se messaggio giusto ritorna recv_port
        else
        {
            printf("UDP: ricevuto messaggio '%s' dal mittente %d\n", buffer, recv_port);
            return recv_port;
        }
    }
    return 0;
}

int send_udp_wait_ack(int socket, char *buffer, int buff_l, int port, char *acked)
{
    char recv_buffer[MAX_UDP_MSG];
    int tries = ACK_TRIES;

    // invia il messaggio
    if (!s_send_udp(socket, buffer, buff_l, port))
    {
        printf("Errore: [S] Impossibile inviare messaggio %s al destinatario %d\n", buffer, port);
    }
    printf("UDP: inviato messaggio '%s' al destinatario %d\n", buffer, port);

    while (!recv_udp(socket, recv_buffer, HEADER_LEN, port, acked) && tries-- > 0)
        ;

    if (tries > -1)
    {
        printf("UDP: ricevuto messaggio '%s' dal destinatario %d\n", buffer, port);
        return 1;
    }

    printf("Errore: [R] impossibile ricevere messaggio %s dal destinatario %d\n", acked, port);
    return 0;
}

int recv_udp_and_ack(int socket, char *buffer, int buff_l, int port, char *correct_header, char *ack_type)
{
    int recv_port;
    int tries = ACK_TRIES;
    // riceve messaggio
    do
    {
        recv_port = recv_udp(socket, buffer, buff_l, port, correct_header);
    } while (!recv_port && tries-- > 0);

    if (tries > -1)
    {
        // manda l'ack
        if (send_ack_udp(socket, ack_type, recv_port))
            return recv_port;
    }

    printf("Errore: [R] impossibile ricevere messaggio %s dal destinatario %d\n", correct_header, port);
    return 0;
}

void send_tcp(int sock, char *buffer, int msg_len)
{
    int ret;

    if (sock < 0)
    {
        printf("Errore: [S] impossibile inviare messaggi su sock: %d\n", sock);
        return;
    }

    ret = send(sock, buffer, msg_len, 0);
    if (ret < 0)
    {
        printf("Errore: [S] impossibile inviare messaggio %s\n", buffer);
    }

    printf("TCP [%d] : inviato messaggio '%s'\n", sock, buffer);
}

int recv_tcp(int sock, char *buffer)
{
    int ret;

    if (sock == -1)
    {
        return -1;
    }

    ret = recv(sock, buffer, MAX_TCP_MSG, 0);
    buffer[ret] = '\0';

    if (ret < 0)
    {
        printf("Errore: [R] impossibile ricevere messaggio\n");
    }
    printf("TCP [%d] : ricevuto messaggio '%s'\n", sock, buffer);

    return ret;
}