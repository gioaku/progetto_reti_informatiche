#include "./util/util_s.h"

// Variabili di stato
int my_port;
struct Date today;
struct Date start_date;

// Udp socket
struct UdpSocket sock;

// Buffer stdin
char command_buffer[MAX_STDIN_S + 1];

// Gestione input
fd_set master;
fd_set readset;
int fdmax;

pid_t time_controller;

int main(int argc, char **argv)
{
    // lettura porta dagli argomenti
    my_port = atoi(argv[1]);

    // inizializzazione date
    update_date(&today);
    printf("Oggi: %02d:%02d:%04d\n", today.d, today.m, today.y);
    start_date = get_start_date(today);
    printf("Start date: %02d:%02d:%04d\n", start_date.d, start_date.m, start_date.y);

    // pulizia set
    FD_ZERO(&master);
    FD_ZERO(&readset);
    
    // nessun peer connesso
    peerlist_init();

    // creazione socket udp
    if (udp_socket_init(&sock, my_port) == -1)
    {
        printf("Errore: impossibile inizializzare socket di ascolto\n");
        exit(0);
    }

    // inizializzo set di descrittori
    FD_SET(sock.id, &master);
    FD_SET(0, &master);
    fdmax = sock.id;

    time_controller = fork();

    if (time_controller == 0)
    {
        while (1)
        {
            if (!update_date(&today))
            {
                // lunghezza del messaggio da inviare
                int msg_len;
                // buffer per la data da inviare
                char date_buffer[DATE_LEN];

                //composizione messaggio
                dtoa(date_buffer, today);
                msg_len = sprintf(sock.buffer, "%s %s", "SET_TDAY", date_buffer);
                sock.buffer[msg_len] = '\0';
                printf("Lista da inviare a tutti: %s (lunga %d byte)\n", sock.buffer, msg_len);

                int i;
                for (i = 0; i < get_n_peers(); i++)
                {
                    if (!send_udp_wait_ack(sock.id, sock.buffer, msg_len, get_port(i), "TDAY_ACK"))
                    {
                        printf("Errore: impossibile comunicare data al peer %d\n", get_port(i));
                    }
                }
            }
            sleep(60);
        }
    }
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
            // buffer per header del messaggio
            char header_buff[HEADER_LEN + 1];

            // ricezione messaggio
            src_port = s_recv_udp(sock.id, sock.buffer, HEADER_LEN);
            sscanf(sock.buffer, "%s", header_buff);
            header_buff[HEADER_LEN] = '\0';

            printf("[UDP]: ricevuto messaggio %s da %d sul socket\n", header_buff, src_port);

            // richiesta di connessione
            if (strcmp(header_buff, "CONN_REQ") == 0)
            {
                // nuovi vicini
                struct Neighbors nbs;
                // lunghezza del messaggio
                int msg_len;
                // buffer per scrivere la data da inviare
                char date_buffer[DATE_LEN + 1];
                if (!peerlist_free())
                {
                    printf("Richiesta rifiutata, lock: %d\n", get_lock());
                    FD_CLR(sock.id, &readset);
                    continue;
                }
                // ack dell'arrivo della richiesta
                if (!send_ack_udp(sock.id, "CONN_ACK", src_port))
                {
                    FD_CLR(sock.id, &readset);
                    continue;
                }

                // inserisco il peer nella lista
                nbs = insert_peer(src_port);

                if (nbs.tot == -1)
                {
                    printf("Impossibile connettere il peer %d\n", src_port);
                    FD_CLR(sock.id, &readset);
                    continue;
                }

                // creo messaggio vicini
                if (nbs.tot == 0)
                    msg_len = sprintf(sock.buffer, "%s", "NBR_LIST");

                else
                    msg_len = sprintf(sock.buffer, "%s %04d %04d", "NBR_LIST", nbs.prev, nbs.next);

                sock.buffer[msg_len] = '\0';

                printf("Lista da inviare a %d: %s (lunga %d byte)\n", src_port, sock.buffer, msg_len);
                print_nbs(src_port, nbs);

                // invio dei vicini
                if (!send_udp_wait_ack(sock.id, sock.buffer, msg_len, src_port, "LIST_ACK"))
                {
                    remove_peer(src_port);
                    printf("Errore: impossibile comunicare vicini al peer\nOperazione abortita\n");
                    continue;
                };

                // composizione messaggio today
                dtoa(date_buffer, today);
                msg_len = sprintf(sock.buffer, "%s %s", "SET_TDAY", date_buffer);
                sock.buffer[msg_len] = '\0';
                printf("Lista da inviare a %d: %s (lunga %d byte)\n", src_port, sock.buffer, msg_len);

                // invio di today
                if (!send_udp_wait_ack(sock.id, sock.buffer, msg_len, src_port, "TDAY_ACK"))
                {
                    remove_peer(src_port);
                    printf("Errore: impossibile comunicare data al peer\nOperazione abortita\n");
                    FD_CLR(sock.id, &readset);
                    continue;
                }

                // composizione messaggio start date
                dtoa(date_buffer, start_date);
                msg_len = sprintf(sock.buffer, "%s %s", "SET_SDAY", date_buffer);
                sock.buffer[msg_len] = '\0';
                printf("Lista da inviare a %d: %s (lunga %d byte)\n", src_port, sock.buffer, msg_len);

                // invio di start date
                if (!send_udp_wait_ack(sock.id, sock.buffer, msg_len, src_port, "SDAY_ACK"))
                {
                    remove_peer(src_port);
                    printf("Errore: impossibile comunicare data al peer\nOperazione abortita\n");
                    FD_CLR(sock.id, &readset);
                    continue;
                }

                // invio aggiornamenti ai vicini
                if (nbs.tot > 0)
                {
                    // comunica modifica a next
                    msg_len = sprintf(sock.buffer, "%s %d", "PRE_UPDT", src_port);
                    sock.buffer[msg_len] = '\0';
                    printf("Lista da inviare a %d: %s (lunga %d byte)\n", nbs.next, sock.buffer, msg_len);
                    send_udp_wait_ack(sock.id, sock.buffer, msg_len, nbs.next, "PREV_ACK");

                    // comunica modifica a prev
                    msg_len = sprintf(sock.buffer, "%s %d", "NXT_UPDT", src_port);
                    sock.buffer[msg_len] = '\0';
                    printf("Lista da inviare a %d: %s (lunga %d byte)\n", nbs.prev, sock.buffer, msg_len);
                    send_udp_wait_ack(sock.id, sock.buffer, msg_len, nbs.prev, "NEXT_ACK");
                }

                // stampa del nuovo numero di peer
                print_peers_number();
            }

            // richiesta di uscita
            else if (strcmp(header_buff, "CLT_EXIT") == 0)
            {
                // vecchi vicini
                struct Neighbors nbs;
                // lunghezza del messaggio
                int msg_len;

                if (!peerlist_free())
                {
                    printf("Richiesta rifiutata, lock: %d\n", get_lock());
                    continue;
                }

                //ack dell'arrivo della richiesta
                if (!send_ack_udp(sock.id, "C_EX_ACK", src_port))
                {
                    FD_CLR(sock.id, &readset);
                    continue;
                }

                // se peer non in lista non faccio nulla
                if (get_position(src_port) == -1)
                {
                    printf("Warning: peer %d non presente nella lista dei peer connessi.\nOperazione abortita\n", src_port);
                    FD_CLR(sock.id, &readset);
                    continue;
                }

                // rimozione peer dalla lista
                nbs = remove_peer(src_port);

                printf("Disconnesso il peer %d dalla rete\n", src_port);

                // se aveva vicini li aggiorno del cambiamento
                if (nbs.tot != 0)
                {
                    // comunica modifica a next
                    msg_len = sprintf(sock.buffer, "%s %d", "PRE_UPDT", nbs.prev);
                    sock.buffer[msg_len] = '\0';
                    printf("Lista da inviare a %d: %s (lunga %d byte)\n", nbs.next, sock.buffer, msg_len);
                    send_udp_wait_ack(sock.id, sock.buffer, msg_len, nbs.next, "PREV_ACK");

                    // comunica modifica a prev
                    msg_len = sprintf(sock.buffer, "%s %d", "NXT_UPDT", nbs.next);
                    sock.buffer[msg_len] = '\0';
                    printf("Lista da inviare a %d: %s (lunga %d byte)\n", nbs.prev, sock.buffer, msg_len);
                    send_udp_wait_ack(sock.id, sock.buffer, msg_len, nbs.prev, "NEXT_ACK");
                }

                // stampa del nuovo numero di peer
                print_peers_number();
            }
            else if (strcmp(header_buff, "LOCK_REQ") == 0)
            {
                send_ack_udp(sock.id, "LOCK_ACK", src_port);
                if (get_position(src_port) != -1)
                {
                    peerlist_lock();
                }
            }
            else if (strcmp(header_buff, "UNLK_REQ") == 0)
            {
                send_ack_udp(sock.id, "UNLK_ACK", src_port);
                if (get_position(src_port) != -1)
                {
                    peerlist_unlock();
                }
            }
        }
        // comando da stdin
        if (FD_ISSET(0, &readset))
        {
            // numero di input
            int input_number;
            // eventuale parametro passato
            int neighbor_peer;
            // buffer per leggere il comando
            char command[MAX_COMMAND_S];

            // leggo il comando
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

            // showneighbors
            else if (strcmp(command, "showneighbors\0") == 0)
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
                int port;
                int first;

                printf("Inizio disconnesione\n");

                first = get_port(0);

                while (!send_udp_wait_ack(sock.id, "SRV_EXIT", HEADER_LEN, first, "S_XT_ACK"))
                {
                    printf("Errore: [S] Impossibile inviare SRV_EXIT al destinatario %d\n", first);
                }
                printf("UDP: inviato messaggio '%s' al destinatario %d\n", sock.buffer, first);

                while ((port = recv_udp_and_ack(sock.id, sock.buffer, HEADER_LEN, -1, "CLT_EXIT", "C_EX_ACK")) != first)
                {
                    if (get_position(port) == -1)
                    {
                        printf("Warning: peer %d non presente nella lista dei peer connessi.\nOperazione abortita\n", port);
                    }
                    // rimozione peer
                    else
                    {
                        remove_peer(port);
                    }
                }

                remove_peer(first);

                close(sock.id);
                _exit(0);
            }

            // errore
            else
            {
                printf("Errore, comando non esistente\n");
                print_server_commands();
            }

            FD_CLR(0, &readset);
        }
        /*
        // Aggiornamento della data corrente
        if (update_date(&today))
        {
            // lunghezza del messaggio da inviare
            int msg_len;
            // buffer per la data da inviare
            char date_buffer[DATE_LEN];

            //composizione messaggio
            dtoa(date_buffer, today);
            msg_len = sprintf(sock.buffer, "%s %s", "SET_TDAY", date_buffer);
            sock.buffer[msg_len] = '\0';
            printf("Lista da inviare a tutti: %s (lunga %d byte)\n", sock.buffer, msg_len);

            int i;
            for (i = 0; i < get_n_peers(); i++)
            {
                if (!send_udp_wait_ack(sock.id, sock.buffer, msg_len, get_port(i), "TDAY_ACK"))
                {
                    printf("Errore: impossibile comunicare data al peer %d\n", get_port(i));
                }
            }
        }
        */
    }
    return 0;
}
