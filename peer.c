#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

// Funzioni di utilita'
#include "./util/util_c.h"
// Gesione dei messaggi
#include "./util/msg.h"
// Gestione rete
#include "./util/neighbors.h"

// Variabili di stato
int my_port;
int server_port;
char today[DATE_LEN + 1];
char first_day[DATE_LEN + 1];

// Buffer stdin
char command_buffer[MAX_STDIN_C];

// Udp socket
struct UdpSocket udp_s;

// Listener socket
struct TcpSocket listener_s;

// Struttura per mantenere lo stato dei vicini
struct Neighbors nbs;

// Variabili per gestire input da socket oppure da stdin
fd_set master;
fd_set readset;
int fdmax;

int main(int argc, char **argv)
{

    // pulizia set
    FD_ZERO(&master);
    FD_ZERO(&readset);

    my_port = atoi(argv[1]);

    // peer non connesso
    nbs.tot = -1;

    // creazione server socket
    if (udp_socket_init(&udp_s, my_port) == -1)
    {
        printf("Error: cannot create server socket\n");
        exit(0);
    }
    // inizializzo listener, next e prev non attivi
    listener_s.id = -1;

    // inizializzo set di descrittori
    FD_SET(0, &master);
    FD_SET(udp_s.id, &master);
    fdmax = udp_s.id;

    // stampa elenco comandi
    print_client_commands();

    // ciclo infinito di funzionamento peer
    while (1)
    {
        readset = master;

        printf("Digita un comando:\n");

        // controllo se c'e' qualcosa pronto
        select(fdmax + 1, &readset, NULL, NULL, NULL);

        // messaggio da stdin
        if (FD_ISSET(0, &readset))
        {
            char command[MAX_COMMAND_C];

            fgets(command_buffer, MAX_STDIN_C, stdin);
            sscanf(command_buffer, "%s", command);

            // help
            if (strcmp(command, "help") == 0)
            {
                int i;
                for (i = 0; i < 5; i++)
                    help_client(i);
            }
            // nbs
            else if (strcmp(command, "nbs") == 0)
            {
                print_nbs(my_port, nbs);
            }
            // start
            else if (strcmp(command, "start") == 0)
            {
                char DS_addr[INET_ADDRSTRLEN]; // Indirizzo IP del server
                char msg_type_buffer[MESS_TYPE_LEN + 1];
                int tmp;

                printf("Inizio connessione...\n");

                // controllo che il peer non sia connesso
                if (nbs.tot != -1)
                {
                    printf("Il peer e' gia' connesso al DS. Il comando non ha effetto\n");
                    continue;
                }

                // lettura e controllo dei parametri
                tmp = sscanf(command_buffer, "%s %s %d", command, DS_addr, &server_port);

                if (tmp != 3 || strcmp(DS_addr, LOCALHOST) != 0 || !valid_port(server_port))
                {
                    printf("Errore: passaggio dei parametri per la connessione non validi\n");
                    help_client(1);
                    continue;
                }

                if (tcp_listener_init(&listener_s, my_port) == -1)
                {
                    printf("Errore: Impossibile creare listener\n");
                    exit(1);
                }

                printf("Creato socket di ascolto in attesa di connessione dai vicini\n");

                // invio richiesta di connessione
                while (!send_udp_wait_ack(udp_s.id, "CONN_REQ", MESS_TYPE_LEN, server_port, "CONN_ACK"))
                {
                    printf("Errore: impossibile inviare richiesta di connessione al server. Riprovare\n");
                    continue;
                }

                // ricevo lista di vicini
                if (!recv_udp_and_ack(udp_s.id, udp_s.buffer, MAX_UDP_MSG, server_port, "NBR_LIST", "LIST_ACK"))
                {
                    printf("Errore: impossibile ricevere la lista di vicini dal server. Riprovare\n");
                    continue;
                }

                tmp = sscanf(udp_s.buffer, "%s %d %d", msg_type_buffer, &nbs.prev, &nbs.next);
                if (tmp == 0)
                {
                    printf("Errore: lista dei vicini ricevuta non valida. Riprovare\n");
                    send_udp_wait_ack(udp_s.id, "CLT_EXIT", MESS_TYPE_LEN, server_port, "C_EX_ACK");
                    continue;
                }
                if (tmp == 1)
                {
                    nbs.tot = 0;
                    printf("Nessun vicino ricevuto. Non ci sono altri peer connessi\n");
                }
                if (tmp == 3)
                {
                    nbs.tot = (nbs.prev == nbs.next) ? 1 : 2;
                    printf("I vicini ricevuti sono prev: %d, next %d\n", nbs.prev, nbs.next);
                }

                // controllo sui parametri letti
                if (!valid_port(nbs.prev) || !valid_port(nbs.next))
                {
                    printf("Errore: lista di vicini ricevuta non valida. Riprovare\n");
                    nbs.tot = -1;
                    continue;
                }

                // ricevo la data di oggi
                if (!recv_udp_and_ack(udp_s.id, udp_s.buffer, MAX_UDP_MSG, server_port, "SET_TDAY", "TDAY_ACK"))
                {
                    printf("Errore: impossibile ricevere la di oggi dal server. Riprovare\n");
                    continue;
                }

                tmp = sscanf(udp_s.buffer, "%s %s", msg_type_buffer, today);
                if (tmp != 2 || !valid_date_s(today))
                {
                    printf("Errore nella ricezione della data odierna %s\n", today);
                    continue;
                }
                printf("Data di oggi ricevuta dal server : %s\n", today);

                // ricevo la prima data
                if (!recv_udp_and_ack(udp_s.id, udp_s.buffer, MAX_UDP_MSG, server_port, "SET_FDAY", "FDAY_ACK"))
                {
                    printf("Errore: impossibile ricevere la di oggi dal server. Riprovare\n");
                    continue;
                }

                tmp = sscanf(udp_s.buffer, "%s %s", msg_type_buffer, first_day);
                if (tmp != 2 || !valid_date_s(first_day))
                {
                    printf("Errore nella ricezione della prima data %s\n", first_day);
                    continue;
                }
                printf("Prima data ricevuta dal server : %s\n", first_day);

                printf("Connessione riuscita\n");

                FD_SET(listener_s.id, &master);
                fdmax = fdmax > listener_s.id ? fdmax : listener_s.id;

                print_nbs(my_port, nbs);
            }

            // add
            else if (strcmp(command, "add") == 0)
            {
                char type;
                int quantity;
                int ret;

                // Se peer non connesso non faccio nulla
                if (server_port == -1)
                {
                    printf("Peer non connesso\n");
                    continue;
                }

                ret = sscanf(command_buffer, "%s %c %d", command, &type, &quantity);
                if (!(type == 't' || type == 'n') || ret != 3 || quantity < 1)
                {
                    printf("Errore nell'inserimento dei dati\n");
                    continue;
                }
                printf("Calling insert_entry(%s, %c, %d)\n", today, type, quantity);
                insert_entry(today, type, quantity);
            }

            // get
            else if (strcmp(command, "get") == 0)
            {
                char aggr;
                char type;
                char period[DATE_IN_LEN * 2 + 2];

                struct tm *from = {0};
                struct tm *to = {0};
                time_t from_time = time(NULL);
                time_t to_time = time(NULL);

                // Se peer non connesso non faccio nulla
                if (server_port == -1)
                {
                    printf("Peer non connesso\n");
                    continue;
                }

                tmp = sscanf(command_buffer, "%s %c %c %s", command, &aggr, &type, period);
                // Numero di parametri

                if (!(tmp == 3 || tmp == 4))
                {
                    printf("Errore nel numero di parametri passati\n");
                    continue;
                }
                // Controllo su aggr e type
                if (!((aggr == 't' || aggr == 'v') && (type == 't' || type == 'n')))
                {
                    printf("Errore nell'inserimento della richiesta\n");
                    continue;
                }
                // Controllo sulle date
                if (tmp == 4)
                {
                    period[DATE_LEN * 2 + 1] = '\0';
                    if (!check_period(period, first_day, today, from, to))
                        continue;
                }

                printf("GET : aggr %c, type %c, between %02d:%02d:%04d and %02d:%02d:%04d\n", aggr, type,from->tm_mday, from->tm_mon, from->tm_year + 1900, to->tm_mday, to->tm_mon, to->tm_year + 1900));

                to_time = mktime(to);

                if (aggr == 't')
                {
                    int tmp;
                    int sum;

                    sum = 0;
                    while (from_time <= to_time)
                    {

                        if (tmp = get_saved_elab(type, from->tm_mday, from->tm_mon, from->tm_year) != -1)
                        {
                            sum += tmp;
                            from->tm_mday++;
                            from_time = mktime(from);
                            continue;
                        }
                        if (nbs.tot == 0)
                        {
                            from->tm_mday++;
                            from_time = mktime(from);
                            continue;
                        }
                        else if (nbs.tot == 1)
                        {
                            char buffer[MAX_TCP_MSG + 1];
                            int msg_len;
                            int sock;
                            
                            // connect con vicino precedente
                            sock = tcp_connect_init(nbs.prev);
                            msg_len = sprintf(buffer, "ELAB_REQ %c %04d_%02d_%02d", type, from->tm_year from->tm_mon, from->tm_mday);
                            
                            // send elab req
                            send(sock, buffer, msg_len, 0);
                            recv(sock, buffer, MAX_TCP_MSG, 0);

                            
                        }

                        sum += tmp;
                        from->tm_mday++;
                        from_time = mktime(from);
                    }
                    printf("Totale di %c nel periodo %s: %d\n", type, period, sum);
                }
                if (aggr == 'v')
                {
                    int old, new;

                    from->tm_day--;
                    from_time = mktime(from);

                    if (!get_saved_elab(type, from->tm_mday, from->tm_mon, from->tm_year, &old))
                    {
                    }

                    while (from_time <= to_time)
                    {
                        if (!get_saved_elab(type, from->tm_mday, from->tm_mon, from->tm_year, &new))
                        {
                        }

                        printf("Variazioni di %c in data %02d:%02d:%04d : %d\n", type, from->tm_mday, from->tm_mon, from->tm_year, new - old);
                        old = new;

                        from->tm_mday++;
                        from_time = mktime(from);
                    }
                }
            }

            // stop
            else if (strcmp(command, "stop") == 0)
            {

                // se connesso disconnettere e inviare le entries a next
                if (nbs.tot != -1)
                {
                    printf("Disconnessione in corso...\n");

                    // invio delle entries
                    // send_entries_to_next();

                    // tentativo di disconnessione
                    if (!send_udp_wait_ack(udp_s.id, "CLT_EXIT", MESS_TYPE_LEN, server_port, "C_EX_ACK"))
                    {
                        printf("Errore disconnessione non riuscita\n");
                        continue;
                    }

                    printf("Disconnessione riuscita\n");
                }

                // chiusura socket e terminazione
                close(listener_s.id);
                close(udp_s.id);
                _exit(0);
            }

            else
                printf("Errore, comando non esistente\n");

            FD_CLR(0, &readset);
        }

        // richiesta di connessione sul socket listener
        if (FD_ISSET(listener_s.id, &readset))
        {
            int new_sd;
            pid_t pid;

            if ((new_sd = accept_connection(listener_s.id)) == -1)
            {
                printf("Errore: impossibile accettare connessione sul listener\n");
            }
            else
            {
                printf("Richiesta di connessione accettata sul socket %d\n", new_sd);
            }

            // fork per gestire la connessione tcp
            pid = fork();
            if (pid == 0)
            {
                int ret;

                close(listener_s.id);

                ret = handle_tcp_socket(new_sd);

                close(new_sd);
                exit(ret);
            }

            close(new_sd);
            FD_CLR(listener_s.id, &readset);
        }

        // messaggio sul socket udp
        if (FD_ISSET(udp_s.id, &readset))
        {
            int src_port;
            char msg_type_buffer[MESS_TYPE_LEN + 1];

            src_port = s_recv_udp(udp_s.id, udp_s.buffer, MAX_UDP_MSG);

            sscanf(udp_s.buffer, "%s", msg_type_buffer);
            msg_type_buffer[MESS_TYPE_LEN] = '\0';

            // server
            if (src_port == server_port)
            {
                printf("Messaggio ricevuto dal server: %s\n", udp_s.buffer);

                // aggiornamento vicino precedente
                if (strcmp(msg_type_buffer, "PRE_UPDT") == 0)
                {
                    int port;

                    sscanf(udp_s.buffer, "%s %d", msg_type_buffer, &port);

                    if (valid_port(port) && s_send_ack_udp(udp_s.id, "PREV_ACK", server_port))
                    {
                        printf("Aggiornamento vicino precedente da %d a %d avvenuto con successo\n", nbs.prev, port);

                        nbs.prev = port;

                        if (nbs.prev == my_port)
                            nbs.tot = 0;
                        else if (nbs.prev == nbs.next)
                            nbs.tot = 1;
                    }
                    else
                    {
                        printf("Errore nell'aggiornamento del vicino precedente da %d a %d\nOperazione annullata\n", nbs.prev, port);
                    }
                }

                // aggiornamento vicino successivo
                else if (strcmp(msg_type_buffer, "NXT_UPDT") == 0)
                {
                    int port;

                    sscanf(udp_s.buffer, "%s %d", msg_type_buffer, &port);

                    if (valid_port(port) && s_send_ack_udp(udp_s.id, "NEXT_ACK", server_port))
                    {
                        printf("Aggiornamento vicino successivo da %d a %d avvenuto con successo\n", nbs.next, port);

                        nbs.next = port;

                        if (nbs.next == my_port)
                            nbs.tot = 0;
                        else if (nbs.prev == nbs.next)
                            nbs.tot = 1;
                    }
                    else
                    {
                        printf("Errore nell'aggiornamento del vicino successivo da %d a %d\nOperazione annullata\n", nbs.next, port);
                    }
                }

                // Notifica cambiamento data
                else if (strcmp(msg_type_buffer, "SET_TDAY") == 0)
                {
                    int tmp;

                    // legge data
                    tmp = sscanf(udp_s.buffer, "%s %s", msg_type_buffer, today);
                    printf("Data ricevuta dal server : %s\n", today);

                    if (tmp != 2 || !valid_date_s(today))
                    {
                        printf("Errore nella ricezione della data odierna");
                        FD_CLR(udp_s.id, &readset);
                        continue;
                    }
                    // invia ack
                    s_send_ack_udp(udp_s.id, "TDAY_ACK", server_port);
                }

                // Notifica chiusura server
                else if (strcmp(msg_type_buffer, "SRV_EXIT") == 0)
                {
                    printf("Il server sta per chiudere\nDisconnessione in corso...\n");

                    // Invia ACK
                    s_send_ack_udp(udp_s.id, "S_XT_ACK", server_port);

                    printf("Disconnessione riuscita\n");

                    // Chiude tutti i socket
                    close(udp_s.id);
                    _exit(0);
                }
            }

            // altro peer sul socket udp
            else
            {
                
                }
                FD_CLR(udp_s.id, &readset);
            }
        }
        return 0;
    }
