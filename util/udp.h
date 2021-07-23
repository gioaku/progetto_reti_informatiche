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


#include "const.h"
#include "neighbors.h"
#include "util.h"

struct UdpSocket{
    int id;
    struct sockaddr_in addr;
    socklen_t addr_len;
    char buffer[MAX_UDP_MSG];
};
int udp_socket_init(struct UdpSocket*, int);
void set_address(struct sockaddr_in*, socklen_t*, int);
int s_recv_udp(int, char*, int);
int s_send_udp(int, char*, int, int);
int s_send_ack_udp(int, char*, int);
int recv_udp(int, char*, int, int, char*);
int send_udp_wait_ack(int, char*, int, int, char*);
int recv_udp_and_ack(int, char*, int, int, char*, char*);