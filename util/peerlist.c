#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "peerlist.h"

// Struttura che contiene i peer connessi alla rete
struct PeerList connected_peers;

void peerlist_init(){
    connected_peers.lock = 0;
    connected_peers.peers = 0;
    connected_peers.list = NULL;
}

void peerlist_unlock()
{
    connected_peers.lock--;
}

void peerlist_lock()
{
    connected_peers.lock++;
}

int peerlist_free(){
    return connected_peers.lock == 0;
};

int get_lock(){
    return connected_peers.lock;
}

int get_n_peers()
{
    return connected_peers.peers;
}

int get_port(int pos)
{
    struct PeerElement *tmp_ptr = connected_peers.list;
    if (connected_peers.peers == 0)
        return -1;

    if (pos >= connected_peers.peers || pos < 0)
        return -1;

    while (pos-- > 0)
    {
        tmp_ptr = tmp_ptr->next;
    }
    return tmp_ptr->port;
}

int get_position(int port)
{
    // se non ci sono peer connessi ritorna errore
    if (connected_peers.peers == 0)
        return -1;

    //scorre la lista per trovare il peer giusto e ne ritorna la posizione
    else
    {
        struct PeerElement *punt;
        int pos = 0;

        punt = connected_peers.list;
        while (pos < connected_peers.peers)
        {
            if (punt->port == port)
                return pos;
            pos++;
            punt = punt->next;
        }
    }

    //peer non trovato
    return -1;
}

struct Neighbors get_neighbors(int peer)
{
    struct Neighbors nbs;
    // se un solo peer ritorna 0 vicini
    if (connected_peers.peers <= 1)
    {
        nbs.tot = 0;
        return nbs;
    }
    // se due peer ritorna lo stesso vicino come next e prev, tot = 1
    else if (connected_peers.peers == 2)
    {
        nbs.tot = 1;
        if (get_port(0) == peer)
        {
            nbs.prev = nbs.next = get_port(1);
        }
        else
        {
            nbs.prev = nbs.next = get_port(0);
        }
        return nbs;
    }

    // altrimenti sfrutto la struttura circolare per trovare i vicini
    else
    {
        struct PeerElement *head, *tail;
        int first;

        first = connected_peers.list->port;
        head = connected_peers.list->next;
        tail = connected_peers.list;
        nbs.tot = 2;

        if (first == peer)
        {
            nbs.next = head->port;
            nbs.prev = get_port(connected_peers.peers - 1);
            return nbs;
        }

        while (head->port != first)
        {
            if (head->port == peer)
            {
                nbs.next = head->next->port;
                nbs.prev = tail->port;
                return nbs;
            }
            tail = head;
            head = head->next;
        }
    }
    
    // se sono qui non ho trovato il peer
    nbs.tot = -1;
    return nbs;
}

struct Neighbors insert_peer(int port)
{
    struct Neighbors nbs;
    struct PeerElement *new = (struct PeerElement *)malloc(sizeof(struct PeerElement));

    if (new == NULL)
    {
        nbs.tot = -1;
        return nbs;
    }

    new->port = port;

    // se primo peer
    if (connected_peers.peers == 0)
    {
        connected_peers.list = new;
        new->next = new;
        connected_peers.peers = 1;

        nbs.tot = 0;
        return nbs;
    }

    // se secondo peer
    else if (connected_peers.peers == 1)
    {
        if (port == connected_peers.list->port)
        {
            free(new);
            nbs.tot = 0;
            return nbs;
        }

        connected_peers.list->next = new;
        new->next = connected_peers.list;
        connected_peers.peers = 2;

        nbs.prev = nbs.next = connected_peers.list->port;
        nbs.tot = 1;

        return nbs;
    }

    // dal terzo in poi inserisce in ordine di porta
    else
    {
        struct PeerElement *head, *tail;

        head = connected_peers.list->next;
        tail = connected_peers.list;

        // esce quando l'ordine di porta viene rispettato
        while ((head->port < port && tail->port > port) || (head->port > tail->port && (port < tail->port || port > head->port)))
        {
            // se peer connesso ritorna i vicini
            if (head->port == port)
            {
                nbs.prev = tail->port;
                nbs.next = head->next->port;
                nbs.tot = 2;
                return nbs;
            }
            tail = head;
            head = head->next;
        }

        tail->next = new;
        new->next = head;
        connected_peers.peers++;

        nbs.prev = tail->port;
        nbs.next = head->port;
        nbs.tot = 2;

        return nbs;
    }

    // errore impossibile connetterlo
    free(new);
    nbs.tot = -1;
    return nbs;
}

struct Neighbors remove_peer(int port)
{
    struct PeerElement *head, *tail;
    struct Neighbors nbs;
    int tmp;

    head = connected_peers.list->next;
    tail = connected_peers.list;
    tmp = connected_peers.peers;

    if (connected_peers.peers == 0)
    {
        nbs.tot = -1;
        return nbs;
    }

    while (head->port != port && tmp-- > 0)
    {
        tail = head;
        head = head->next;
    }

    if (head->port == port)
    {
        if (tail->port == port)
        {
            nbs.tot = 0;
            connected_peers.peers = 0;
        }
        else
        {
            if (head == connected_peers.list)
            {
                connected_peers.list = head->next;
            }
            nbs.prev = tail->port;
            nbs.next = head->next->port;
            nbs.tot = (nbs.prev == nbs.next) ? 1 : 2;
            tail->next = head->next;
            connected_peers.peers--;
        }
        free(head);
    }

    else
    {
        nbs.tot = -1;
    }

    return nbs;
}

void print_peers_number()
{
    printf("Ci sono [ %d ] peer connessi alla rete\n", connected_peers.peers);
}

void print_peers()
{
    if (connected_peers.peers == 0)
    {
        printf("Non ci sono peers connessi alla rete\n");
    }
    else
    {
        struct PeerElement *punt;
        int tmp;

        punt = connected_peers.list;
        printf("I peer connessi alla rete sono:\n");
        printf("\t -- \tpeer\t -- \n");
        for (tmp = 0; tmp < connected_peers.peers; tmp++)
        {
            printf("\t -- \t%d\t -- \n", punt->port);
            punt = punt->next;
        }
    }
}

void print_peers_nbs()
{
    if (connected_peers.peers == 0)
    {
        printf("Non ci sono peers connessi alla rete\n");
    }
    else if (connected_peers.peers == 1)
    {
        printf("Solo un peer connesso alla rete:\n");
        printf("\t -- \tpeer\t -- \n");
        printf("\t -- \t%d\t -- \n", connected_peers.list->port);
    }
    else
    {
        struct PeerElement *head, *tail;
        int first;
        printf("I peer connessi alla rete sono:\n");
        printf("\tprev -- peer -- next\n");

        first = connected_peers.list->port;
        head = connected_peers.list->next;
        tail = connected_peers.list;
        printf("\t%d -> %d -> %d\n", get_port(connected_peers.peers - 1), first, head->port);

        while (head->port != first)
        {
            printf("\t%d -> %d -> %d\n", tail->port, head->port, head->next->port);
            tail = head;
            head = head->next;
        }
    }
}
