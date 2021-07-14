#include <stdio.h>
#include "neighbors.h"
// Stampa i vicini nbs di peer
void print_nbs(int peer, struct Neighbors nbs)
{
    if (nbs.tot == -1)
    {
        printf("Il peer %d non connesso alla rete\n", peer);
    }
    if(nbs.tot == 0){
        printf("Il peer %d non ha vicini. Unico peer connesso\n", peer);
    }
    else{
        printf("Vicini di %d:\nprev\tpeer\tnext \n%d -> %d -> %d\n", peer, nbs.prev, peer, nbs.next);
    }
}
