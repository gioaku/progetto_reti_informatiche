#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

// Funzioni di utilita'
#include "./util/util_c.h"
#include "./util/util.h"
// Gesione dei messaggi
#include "./util/msg.h"
// Gestione rete
#include "./util/neighbors.h"
// Costanti
#include "./util/const.h"

// Variabili di stato
int my_port;
char today[DATE_LEN + 1];

// Buffer stdin
char command_buffer[MAX_STDIN_C];

// Server socket
struct UdpSocket server_s;

// Listener socket
struct TcpSocket listener_s;

// Struttura per mantenere lo stato dei vicini
struct Neighbors nbs;

// Precedente
struct TcpSocket prev_s;

// Successivo
struct TcpSocket next_s;

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
    if (udp_socket_init(&server_s, my_port) == -1)
    {
        printf("Error: cannot create server socket\n");
        exit(0);
    }
    // inizializzo listener, next e prev non attivi
    listener_s.id = prev_s.id = next_s.id = -1;

    // inizializzo set di descrittori
    FD_SET(0, &master);
    FD_SET(server_s.id, &master);
    fdmax = server_s.id;

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
                tmp = sscanf(command_buffer, "%s %s %d", command, DS_addr, &server_s.port);

                if (tmp != 3 || strcmp(DS_addr, LOCALHOST) != 0 || !valid_port(server_s.port))
                {
                    printf("Errore: passaggio dei parametri per la connessione non validi\n");
                    help_client(1);
                    continue;
                }

                if (tcp_listener_init(&listener_s, my_port) == -1)
                {
                    printf("Errore: Impossibile creare listener\n");
                    exit(0);
                }

                printf("Creato socket di ascolto in attesa di connessione dai vicini\n");

                // invio richiesta di connessione
                while (!send_udp_wait_ack(server_s.id, "CONN_REQ", MESS_TYPE_LEN, server_s.port, "CONN_ACK"))
                {
                    printf("Errore: impossibile inviare richiesta di connessione al server. Riprovare\n");
                    continue;
                }

                // ricevo lista di vicini
                if (!recv_udp_and_ack(server_s.id, server_s.buffer, MAX_UDP_MSG, server_s.port, "NBR_LIST", "LIST_ACK"))
                {
                    printf("Errore: impossibile ricevere la lista di vicini dal server. Riprovare\n");
                    continue;
                }

                tmp = sscanf(server_s.buffer, "%s %d %d", msg_type_buffer, &nbs.prev, &nbs.next);
                if (tmp == 0)
                {
                    printf("Errore: lista dei vicini ricevuta non valida. Riprovare\n");
                    send_udp_wait_ack(server_s.id, "CLT_EXIT", MESS_TYPE_LEN, server_s.port, "C_EX_ACK");
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
                if (!recv_udp_and_ack(server_s.id, server_s.buffer, MAX_UDP_MSG, server_s.port, "SET_DATE", "DATE_ACK"))
                {
                    printf("Errore: impossibile ricevere la di oggi dal server. Riprovare\n");
                    continue;
                }

                tmp = sscanf(server_s.buffer, "%s %s", msg_type_buffer, today);
                printf("Data ricevuta dal server : %s\n", today);

                printf("Connessione riuscita\n");

                FD_SET(listener_s.id, &master);
                fdmax = fdmax > listener_s.id ? fdmax : listener_s.id;

                print_nbs(my_port, nbs);
            }
            /*
            // add
            else if (strcmp(command, "add") == 0)
            {
                char type;
                int quantity;
                char new_entry[ENTR_W_TYPE + 1];

                // Se peer non connesso non faccio nulla
                if (server_port == -1)
                {
                    printf("Peer non connesso\n");
                    continue;
                }

                tmp = sscanf(stdin_buff, "%s %c %d", command, &type, &quantity);
                if (!(type == 't' || type == 'n') || tmp != 3 || quantity < 1)
                {
                    printf("Errore nell'inserimento dei dati\n");
                    continue;
                }
                
                // Controllo che nessuno stia eseguendo una get
                if(check_g_lock()){
                    printf("Comando %s non eseguibile, riprova piu' tardi\n", command);
                    continue;
                }
                ret = sprintf(new_entry, "%s %c", "NEW_ENTR", type);
                new_entry[ret] = '\0';

                send_UDP(listener_socket, new_entry, ret, server_port, "ENEW_ACK");

                insert_entry(type, quantity);
            }

            // get
            else if (strcmp(command, "get") == 0)
            {
                char aggr;
                char type;
                char bound[2][DATE_LEN + 1];
                int tot_entr;
                int peer_entr;
                int sum_entr;
                char get_buffer[MAX_SUM_ENTRIES]; // Ricevere il messaggio

                // Se peer non connesso non faccio nulla
                if (server_port == -1)
                {
                    printf("Peer non connesso\n");
                    continue;
                }

                tmp = sscanf(stdin_buff, "%s %c %c %s %s", command, &aggr, &type, bound[0], bound[1]);
                // Numero di parametri
                bound[0][DATE_LEN] = '\0';
                bound[1][DATE_LEN] = '\0';

                if (!(tmp == 3 || tmp == 5))
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
                if (tmp == 5)
                {
                    if (!check_dates(bound[0], bound[1], aggr))
                        continue;
                }

                printf("Controlli superati!\n");

                // Se c'e' qualche altro peer che sta eseguendo la get, mi fermo
                if(get_lock()){
                    printf("Comando get non eseguibile in questo momento, riprova piu' tardi\n");
                    continue;
                }
                tot_entr = -1;
                peer_entr = -1;
                sum_entr = -1;

                // Riempio i buffer delle date nel caso non siano state inserite dall'utente
                if (tmp == 3)
                {
                    strcpy(bound[0], "*");
                    strcpy(bound[1], "*");
                }

                // Se la data di fine e' oggi va bene l'asterisco
                if (is_today(bound[1]))
                    strcpy(bound[1], "*");


                // Se la data di inizio e' oggi, ho finito perche' veniva solo richiesto il totale di oggi
                if (!is_today(bound[0]))
                {
                    char get_past_aggr[MAX_PAST_AGGR];
                    int aggr_entries;

                    printf("Mi serve lo storico dei dati\n");
                    tmp = sprintf(get_past_aggr, "%s %s %s", "AGGR_GET", bound[0], bound[1]);
                    get_past_aggr[tmp] = '\0';
                    send_UDP(listener_socket, get_past_aggr, tmp, time_port, "AGET_ACK");

                    printf("Aspetto i dati dal time server\n");
                    // Sfrutto get_past_aggr
                    recv_UDP(listener_socket, get_past_aggr, MAX_PAST_AGGR, time_port, "AGGR_CNT", "ACNT_ACK");
                    sscanf(get_past_aggr + 9, "%d", &aggr_entries);
                    printf("Entrate da ricevere: %d\n", aggr_entries);

                    while (aggr_entries--)
                    {
                        // Sfrutto stdin_buffer che e' della lunghezza giusta
                        recv_UDP(listener_socket, stdin_buff, 40, time_port, "AGGR_ENT", "AENT_ACK");
                        printf("Ricevuta stringa %s\n", stdin_buff + 9);
                        // Memorizzo le entrate in un buffer temporaneo
                        insert_temp(stdin_buff + 9);
                    }

                    // Eseguo l'operazione di stampa finale
                    print_results(aggr, type, sum_entr, bound[0], bound[1]);
                }

                // Rilascio risorsa per la get
                send_UDP(listener_socket, "GET_UNLK", MESS_TYPE_LEN, server_port, "GNLK_ACK");
            }
*/

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
                    if (!send_udp_wait_ack(server_s.id, "CLT_EXIT", MESS_TYPE_LEN, server_s.port, "C_EX_ACK"))
                    {
                        printf("Errore disconnessione non riuscita\n");
                        continue;
                    }

                    printf("Disconnessione riuscita\n");
                }

                // chiusura socket e terminazione
                close(server_s.id);
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

            if ((new_sd = accept_nb_connection(listener_s.id, nbs, &prev_s, &next_s)) == -1)
            {
                printf("Errore: impossibile accettare richiesta di connessione\n");
                FD_CLR(listener_s.id, &readset);
                continue;
            }

            fdmax = new_sd > fdmax ? new_sd : fdmax;
            FD_SET(new_sd, &master);
            FD_CLR(listener_s.id, &readset);
        }

        // messaggio sul server socket
        if (FD_ISSET(server_s.id, &readset))
        {
            int src_port;
            char msg_type_buffer[MESS_TYPE_LEN + 1];

            src_port = s_recv_udp(server_s.id, server_s.buffer, MAX_UDP_MSG);

            sscanf(server_s.buffer, "%s", msg_type_buffer);
            msg_type_buffer[MESS_TYPE_LEN] = '\0';

            // server
            if (src_port == server_s.port)
            {
                printf("Messaggio ricevuto dal server: %s\n", server_s.buffer);

                // aggiornamento vicino precedente
                if (strcmp(msg_type_buffer, "PRE_UPDT") == 0)
                {
                    int port;

                    sscanf(server_s.buffer, "%s %d", msg_type_buffer, &port);

                    if (valid_port(port) && s_send_ack_udp(server_s.id, "PREV_ACK", server_s.port))
                    {
                        printf("Aggiornamento vicino precedente da %d a %d avvenuto con successo\n", nbs.prev, port);

                        printf("Connessione con vecchio vicino precendente %d avvenuta con successo\n", nbs.prev);

                        nbs.prev = port;

                        if (nbs.prev == my_port)
                            nbs.tot = 0;
                        else
                        {
                            if (nbs.prev == nbs.next)
                                nbs.tot = 1;
                            if (tcp_connect_init(nbs.prev, &prev_s) == -1)
                            {
                                printf("Errore: impossibile connettersi al nuovo vicino precedente %d\n", nbs.prev);
                                continue;
                            }

                            printf("Connessione con nuovo vicino precendente %d avvenuta con successo\n", nbs.prev);
                        }
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

                    sscanf(server_s.buffer, "%s %d", msg_type_buffer, &port);

                    if (valid_port(port) && s_send_ack_udp(server_s.id, "NEXT_ACK", server_s.port))
                    {
                        printf("Aggiornamento vicino successivo da %d a %d avvenuto con successo\n", nbs.next, port);
                        if (close(next_s.id) == -1)
                        {
                            printf("Errore: impossibile chiudere connessione con il vecchio vicino successivo\n");
                        }

                        printf("Connessione con vecchio vicino successivo %d avvenuta con successo\n", nbs.next);

                        nbs.next = port;

                        if (nbs.next == my_port)
                            nbs.tot = 0;
                        else
                        {

                            if (nbs.prev == nbs.next)
                                nbs.tot = 1;
                            if (tcp_connect_init(nbs.next, &next_s) == -1)
                            {
                                printf("Errore: impossibile connettersi al nuovo vicino successivo %d\n", nbs.next);
                                continue;
                            }
                            printf("Connessione con nuovo vicino precendente %d avvenuta con successo\n", nbs.next);
                        }
                    }
                    else
                    {
                        printf("Errore nell'aggiornamento del vicino successivo da %d a %d\nOperazione annullata\n", nbs.next, port);
                    }
                }
                
                // Notifica cambiamento data
                else if (strcmp(msg_type_buffer, "SET_DATE") == 0)
                {
                    int tmp;

                    // legge data
                    tmp = sscanf(server_s.buffer, "%s %s", msg_type_buffer, today);
                    printf("Data ricevuta dal server : %s\n", today);

                    // invia ack
                    s_send_ack_udp(server_s.id, "DATE_ACK", server_s.port);
                }

                // Notifica chiusura server
                else if (strcmp(msg_type_buffer, "SRV_EXIT") == 0)
                {
                    printf("Il server sta per chiudere\nDisconnessione in corso...\n");

                    // Invia ACK
                    s_send_ack_udp(server_s.id, "S_XT_ACK", server_s.port);

                    printf("Disconnessione riuscita\n");

                    // Chiude tutti i socket
                    close(server_s.id);
                    _exit(0);
                }
            }
            FD_CLR(server_s.id, &readset);
        }
    }
    return 0;
}
