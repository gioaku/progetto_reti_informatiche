#ifndef MSG
#define MSG
#include "const.h"
#include "neighbors.h"

struct UdpSocket{
    int id;
    struct sockaddr_in addr;
    socklen_t addr_len;
    char buffer[MAX_UDP_MSG];
};

struct TcpSocket{
    int id;
    struct sockaddr_in addr;
    socklen_t addr_len;
    char buffer[MAX_TCP_MSG];    
};


void set_address(struct sockaddr_in*, socklen_t*, int);
int udp_socket_init(struct UdpSocket*, int);
int tcp_listener_init(struct TcpSocket*, int);
int accept_connection(int);
int tcp_connect_init(int);
int s_recv_udp(int, char*, int);
int s_send_udp(int, char*, int, int);
int s_send_ack_udp(int, char*, int);
int recv_udp(int, char*, int, int, char*);
int send_udp_wait_ack(int, char*, int, int, char*);
int recv_udp_and_ack(int, char*, int, int, char*, char*);
#endif
