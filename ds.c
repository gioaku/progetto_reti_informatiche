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

int my_port;
char today[DATE_LEN + 1];

// Udp socket
struct UdpSocket sock;

// Buffer stdin
char command_buffer[MAX_STDIN_S];

// Gestione input
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

    my_port = atoi(argv[1]);
    // inizializzazione data
    update_date(today);

    // creazione socket di ascolto
    if (udp_socket_init(&sock, my_port) == -1)
    {
        printf("Error: udp init gone wrong");
        exit(0);
    }

    // inizializzo set di descrittori
    FD_SET(sock.id, &master);
    FD_SET(0, &master);
    fdmax = sock.id;

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
        if (FD_ISSET(sock.id, &readset))
        {
            // porta del peer mittente
            int src_port;
            // Buffer su cui ricevere messaggio di richiesta connessione
            char msg_type_buffer[MESS_TYPE_LEN + 1];

            // ricezione messaggio
            src_port = s_recv_udp(sock.id, sock.buffer, MESS_TYPE_LEN);
            sscanf(sock.buffer, "%s", msg_type_buffer);
            msg_type_buffer[MESS_TYPE_LEN] = '\0';

            printf("Arrivato messaggio %s da %d sul socket\n", msg_type_buffer, src_port);

            // richiesta di connessione
            if (strcmp(msg_type_buffer, "CONN_REQ") == 0)
            {
                struct Neighbors nbs; // Variabile per salvare eventuali vicini
                int msg_len;          // Variabile per la lunghezza del messaggio da inviare al peer

                // ack dell'arrivo della richiesta
                if (!s_send_ack_udp(sock.id, "CONN_ACK", src_port))
                {
                    printf("Errore: impossibile inviare ack per la richiesta di connessione\n");
                    continue;
                }

                // inserisco il peer nella lista
                nbs = insert_peer(src_port);

                if (nbs.tot == -1)
                {
                    printf("Impossibile connettere il peer\n");
                    // Uscita
                    FD_CLR(sock.id, &readset);
                    continue;
                }

                // compongo la lista
                if (nbs.tot == 0)
                    msg_len = sprintf(sock.buffer, "%s", "NBR_LIST");
                else
                    msg_len = sprintf(sock.buffer, "%s %d %d", "NBR_LIST", nbs.prev, nbs.next);

                printf("Lista da inviare a %d: %s (lunga %d byte)\n", src_port, sock.buffer, msg_len);
                print_nbs(src_port, nbs);

                // invio dei vicini
                if (!send_udp_wait_ack(sock.id, sock.buffer, msg_len, src_port, "LIST_ACK"))
                {
                    remove_peer(src_port);
                    printf("Errore: impossibile comunicare vicini al peer\nOperazione abortita\n");
                    continue;
                };

                msg_len = sprintf(sock.buffer, "%s %s", "SET_DATE", today);
                printf("Lista da inviare a %d: %s (lunga %d byte)\n", src_port, sock.buffer, msg_len);

                if (!send_udp_wait_ack(sock.id, sock.buffer, msg_len, src_port, "DATE_ACK"))
                {
                    remove_peer(src_port);
                    printf("Errore: impossibile comunicare data al peer\nOperazione abortita\n");
                    continue;
                }

                // invio aggiornamenti vicini
                if (nbs.tot > 0)
                {
                    msg_len = sprintf(sock.buffer, "%s %d", "PRE_UPDT", src_port);
                    printf("Lista da inviare a %d: %s (lunga %d byte)\n", nbs.next, sock.buffer, msg_len);
                    send_udp_wait_ack(sock.id, sock.buffer, msg_len, nbs.next, "PREV_ACK");

                    msg_len = sprintf(sock.buffer, "%s %d", "NXT_UPDT", src_port);
                    printf("Lista da inviare a %d: %s (lunga %d byte)\n", nbs.prev, sock.buffer, msg_len);
                    send_udp_wait_ack(sock.id, sock.buffer, msg_len, nbs.prev, "NEXT_ACK");
                }
                print_peers_number();
            }

            // Richiesta di uscita
            if (strcmp(msg_type_buffer, "CLT_EXIT") == 0)
            {
                // Variabili per salvare informazioni temporanee
                struct Neighbors nbs;
                int msg_len;

                printf("Ricevuto messaggio di richiesta di uscita da %d\n", src_port);
                s_send_ack_udp(sock.id, "C_EX_ACK", src_port);

                // Se il peer per qualche motivo non e' in lista non faccio nulla
                if (get_position(src_port) == -1)
                {
                    printf("Errore: peer %d non presente nella lista dei peer connessi.\nOperazione abortita\n", src_port);
                    FD_CLR(sock.id, &readset);
                    continue;
                }

                nbs = remove_peer(src_port);

                printf("Disconnesso il peer %d dalla rete\n", src_port);

                if (nbs.tot == 0)
                {
                    printf("Disconnesso l'ultimo peer\n");
                }
                else
                {
                    msg_len = sprintf(sock.buffer, "%s %d", "PRE_UPDT", nbs.prev);
                    printf("Lista da inviare a %d: %s (lunga %d byte)\n", nbs.next, sock.buffer, msg_len);
                    send_udp_wait_ack(sock.id, sock.buffer, msg_len, nbs.next, "PREV_ACK");

                    msg_len = sprintf(sock.buffer, "%s %d", "NXT_UPDT", nbs.next);
                    printf("Lista da inviare a %d: %s (lunga %d byte)\n", nbs.prev, sock.buffer, msg_len);
                    send_udp_wait_ack(sock.id, sock.buffer, msg_len, nbs.prev, "NEXT_ACK");

                    print_peers_number();
                }
            }
        }
        // Gestione comandi da stdin
        if (FD_ISSET(0, &readset))
        {
            // Parsing dell'input
            int input_number;
            int neighbor_peer;
            char command[MAX_COMMAND_S];

            fgets(command_buffer, MAX_STDIN_S, stdin);
            input_number = sscanf(command_buffer, "%s %d", command, &neighbor_peer);
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
                int port = get_port(0);

                // rimozione di tutti i peer previo avviso
                while (port != -1)
                {
                    printf("Invio SRV_EXIT a %d\n", port);

                    // invio messaggio
                    if (!send_udp_wait_ack(sock.id, "SRV_EXIT", MESS_TYPE_LEN, port, "S_XT_ACK"))
                    {
                        printf("Errore: impossibile disconnettere il peer %d\n", port);
                        // spero bene per lui
                    }

                    // rimozione peer
                    port = remove_first_peer();
                }
                close(sock.id);
                _exit(0);
            }

            // errore
            else
            {
                printf("Errore, comando non esistente\n");
            }

            FD_CLR(0, &readset);
        }

        if (update_date(today))
        {
            int msg_len;
            msg_len = sprintf(sock.buffer, "%s %s", "SET_DATE", today);
            printf("Lista da inviare a tutti: %s (lunga %d byte)\n", sock.buffer, msg_len);

            int i;
            for (i = 0; i < get_n_peers(); i++)
            {
                if (!send_udp_wait_ack(sock.id, sock.buffer, msg_len, get_port(i), "DATE_ACK"))
                {
                    printf("Errore: impossibile comunicare data al peer %d\n", get_port(i));
                }
            }
        }
    }

    return 0;
}
