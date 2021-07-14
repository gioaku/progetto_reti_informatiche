struct PeerElement
{
    int port;
    struct PeerElement *next;
};
struct PeerList
{
    int peers;
    int mutex;
    struct PeerElement *list; 
};

void peer_file_signal();
int peer_file_wait();
int get_port(int);
int get_position(int);
struct Neighbors get_neighbors(int);
struct Neighbors insert_peer(int);
struct Neighbors remove_peer(int);
void print_peers_number();
void print_peers();
void print_peers_nbs();