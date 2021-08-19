#include "neighbors.h"
#include "util.h"

struct PeerElement
{
    int port;
    struct PeerElement *next;
};
struct PeerList
{
    int peers;
    int lock;
    struct PeerElement *list; 
};

// Inizializza la struttura dei peer come vuota
void peerlist_init();

// Decrementa la variabile lock
void peerlist_unlock();

// Incrementa la variabile lock
void peerlist_lock();

// Ritorna 1 se lock == 0 - ritorna 0 altrimenti
int peerlist_free();

// Ritorna la variabile lock
int get_lock();

// Ritorna connected_peers.peers
int get_n_peers();

// Ritorna la porta del peer in posizione pos
int get_port(int pos);

// Ritorna la posizione del peer port - ritorna -1 se non trovato
int get_position(int port);

// Trova i vicini di un peer e li restituisce
struct Neighbors get_neighbors(int peer);

// Inserisce un peer nel file dei peer e ritorna i nuovi vicini
struct Neighbors insert_peer(int port);

// Rimuove il peer port dalla lista di quelli connessi - ritorna i vicini prima della rimozione
struct Neighbors remove_peer(int port);

// Stampa il numero di peer connessi
void print_peers_number();

// Stampa i peer connessi alla rete (comando showpeers)
void print_peers();

// Stampa i vicini di tutti i peer (comando showneighbors)
void print_peers_nbs();