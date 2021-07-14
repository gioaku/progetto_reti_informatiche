#include <stdio.h>
#include "neighbors.h"
// Stampa i vicini nbs di peer
void print_nbs(int peer, struct Neighbors nbs)
{
    if (nbs.tot == -1)
    {
        printf("Il peer %d non connesso alla rete", peer);
    }
    if(nbs.tot == 0){
        printf("Il peer %d non ha vicini, unico peer connesso", peer);
    }
    else{
        printf("Vicini di %d:\nprev\tpeer\tnext \n%d -> %d -> %d", peer, nbs.prev, peer, nbs.next);
    }
}
