void set_address(struct sockaddr_in*, socklen_t*, int);
int udp_socket_init(struct sockaddr_in*, socklen_t*,int);
int s_recv_udp(int, char*, int);
int s_send_udp(int, char*, int, int)
int s_send_ack_udp(int, char*, int);
int recv_udp(int, char*, int, int, char*);
int send_udp_wait_ack(int, char*, int, int, char*);
int recv_udp_and_ack(int, char*, int, int, char*, char*);