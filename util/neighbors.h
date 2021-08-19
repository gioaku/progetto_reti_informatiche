#ifndef NEIGHBORS
#define NEIGHBORS

struct Neighbors
{
    // vicino precedente
    int prev;
    // vicino successivo
    int next;
    // [0-2] numero di vicini, -1 se peer non connesso
    int tot;
};

// Stampa i vicini nbs di peer
void print_nbs(int, struct Neighbors);

#endif