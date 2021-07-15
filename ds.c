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

// Funzioni di utilita'
#include "./util/util_s.h"
#include "./util/util.h"
// Gestione del file con i peer connessi alla rete
#include "./util/peer_file.h"
// Gesione dei messaggi
#include "./util/msg.h"
// Costanti
#include "./util/const.h"

// Variabili
int server_socket;              // Socket su cui il server riceve messaggi dai peer
struct sockaddr_in server_addr; // Struttura per gestire il socket
socklen_t server_len;

char command_buffer[MAX_STDIN_S]; // Buffer su cui salvare i comandi provenienti da stdin
char socket_buffer[MAX_SOCKET_RECV];
char recv_buffer[MESS_TYPE_LEN + 1]; // Buffer su cui ricevere messaggio di richiesta connessione

// Gestione input da stdin oppure da socket
fd_set master;
fd_set readset;
int fdmax;

// Mutua esclusione per la start, get e stop
// Ogni entrata e uscita dei peer deve essere atomica
// Voglio un grafo costante durante quando un peer esegue una flood for entries

int main(int argc, char **argv)
{
    // pulizia set
    FD_ZERO(&master);
    FD_ZERO(&readset);

    // creazione socket di ascolto
    server_socket = udp_socket_init(&server_addr, &server_len, atoi(argv[1]));

    // inizializzo set di descrittori
    FD_SET(server_socket, &master);
    FD_SET(0, &master);
    fdmax = server_socket;

    // stapa elenco comandi
    print_server_commands();

    // ciclo infinito di funzionamento ds
    while (1)
    {
        readset = master;

        printf("Digita un comando:\n");

        // controllo se c'e' qualcosa pronto
        select(fdmax + 1, &readset, NULL, NULL, NULL);

        // messaggio da un peer
        if (FD_ISSET(server_socket, &readset))
        {
            // porta del peer mittente
            int peer_port;

            // ricezione messaggio
            peer_port = s_recv_udp(server_socket, socket_buffer, MESS_TYPE_LEN);
            sscanf(socket_buffer, "%s", recv_buffer);
            recv_buffer[MESS_TYPE_LEN] = '\0';

            printf("Arrivato messaggio %s da %d sul socket\n", socket_buffer, peer_port);

            // richiesta di connessione
            if (strcmp(recv_buffer, "CONN_REQ") == 0)
            {
                struct Neighbors nbs;           // Variabile per salvare eventuali vicini
                char list_buffer[MAX_LIST_LEN]; // Buffer per invio vicini al peer
                int buff_len;                   // Variabile per la lunghezza del messaggio da inviare al peer

                // ack dell'arrivo della richiesta
                if (!s_send_ack_udp(server_socket, "CONN_ACK", peer_port))
                {
                    printf("Errore: impossibile inviare ack per la richiesta di connessione\n");
                    continue;
                }

                // inserisco il peer nella lista
                nbs = insert_peer(peer_port);

                if (nbs.tot == -1)
                {
                    printf("Impossibile connettere il peer\n");
                    // Uscita
                    FD_CLR(server_socket, &readset);
                    continue;
                }

                print_nbs(peer_port, nbs);

                // compongo la lista
                if (nbs.tot == 0)
                    buff_len = sprintf(list_buffer, "%s %d %d", "NBR_LIST", htonl(-1), htonl(-1));
                else
                    buff_len = sprintf(list_buffer, "%s %d %d", "NBR_LIST", htonl(nbs.prev), htonl(nbs.next));

                printf("Lista da inviare a %d: %s (lunga %d byte)\n", peer_port, list_buffer, buff_len);

                // invio dei vicini
                if (!send_udp_wait_ack(server_socket, list_buffer, buff_len, peer_port, "LIST_ACK"))
                {
                    remove_peer(peer_port);
                    printf("Errore: impossibile comunicare vicini al peer\nOperazione abortita\n");
                    continue; 
                };

                // invio aggiornamenti vicini
                if (nbs.tot > 0)
                {
                    buff_len = sprintf(list_buffer, "%s %d", "PRE_UPDT", htonl(peer_port));
                    printf("Lista da inviare a %d: %s (lunga %d byte)\n", nbs.next, list_buffer, buff_len);
                    send_udp_wait_ack(server_socket, list_buffer, buff_len, nbs.next, "PREV_ACK");

                    buff_len = sprintf(list_buffer, "%s %d", "NXT_UPDT", htonl(peer_port));
                    printf("Lista da inviare a %d: %s (lunga %d byte)\n", nbs.prev, list_buffer, buff_len);
                    send_udp_wait_ack(server_socket, list_buffer, buff_len, nbs.prev, "NEXT_ACK");
                }
                print_peers_number();
            }

            // Richiesta di uscita
            if (strcmp(recv_buffer, "CLT_EXIT") == 0)
            {
                // Variabili per salvare informazioni temporanee
                struct Neighbors nbs;
                char list_buffer[MAX_LIST_LEN];
                int buff_len;

                printf("Ricevuto messaggio di richiesta di uscita da %d\n", peer_port);
                s_send_ack_udp(server_socket, "C_EX_ACK", peer_port);

                // Se il peer per qualche motivo non e' in lista non faccio nulla
                if (get_position(peer_port) == -1)
                {
                    printf("Errore: peer %d non presente nella lista dei peer connessi.\nOperazione abortita\n", peer_port);
                    FD_CLR(server_socket, &readset);
                    continue;
                }

                nbs = remove_peer(peer_port);

                printf("Disconnesso il peer %d dalla rete\n", peer_port);

                if (nbs.tot == 0)
                {
                    printf("Disconnesso l'ultimo peer\n");
                }
                else
                {
                    buff_len = sprintf(list_buffer, "%s %d", "PRE_UPDT", nbs.prev);
                    printf("Lista da inviare a %d: %s (lunga %d byte)\n", nbs.next, list_buffer, buff_len);
                    send_udp_wait_ack(server_socket, list_buffer, buff_len, nbs.next, "PREV_ACK");

                    buff_len = sprintf(list_buffer, "%s %d", "NXT_UPDT", nbs.next);
                    printf("Lista da inviare a %d: %s (lunga %d byte)\n", nbs.prev, list_buffer, buff_len);
                    send_udp_wait_ack(server_socket, list_buffer, buff_len, nbs.prev, "NEXT_ACK");

                    print_peers_number();
                }
            }
        }
        // Gestione comandi da stdin
        if (FD_ISSET(0, &readset))
        {
            // Parsing dell'input
            int neighbor_peer;
            int input_number;
            char command[MAX_COMMAND_S];

            fgets(command_buffer, MAX_COMMAND_S, stdin);
            input_number = sscanf(command_buffer, "%s %d", command, &neighbor_peer);
            printf("Ricevuti %d parametri da stdin: %s, %d", input_number, command, neighbor_peer);

            // help
            if (strcmp(command, "help\0") == 0)
            {
                print_server_commands();
            }

            // showpeers
            else if (strcmp(command, "showpeers\0") == 0)
            {
                print_peers();
            }

            // showneighbor
            else if (strcmp(command, "showneighbor\0") == 0)
            {

                if (input_number == 2)
                {
                    print_nbs(neighbor_peer, get_neighbors(neighbor_peer));
                }
                else
                {
                    print_peers_nbs();
                }
            }

            // esc
            else if (strcmp(command, "esc\0") == 0)
            {
                printf("Invio ai peer i messaggi di disconnessione\n");

                // rimozione di tutti i peer previo avviso
                while (get_port(0) != -1)
                {
                    printf("Invio SRV_EXIT a %d\n", get_port(0));

                    // invio messaggio
                    if (send_udp_wait_ack(server_socket, "SRV_EXIT", MESS_TYPE_LEN, get_port(0), "S_XT_ACK"))
                    {
                        printf("Errore: impossibile disconnettere il peer %d\n", get_port(0));
                        // spero bene per lui
                    }

                    // rimozione peer
                    remove_peer(0);
                }
                close(server_socket);
                _exit(0);
            }

            // errore
            else
            {
                printf("Errore, comando non esistente\n");
            }

            FD_CLR(0, &readset);
        }
    }

    return 0;
}
