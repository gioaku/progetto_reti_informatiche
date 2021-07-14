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

// Variabili
int my_port;
int listener_socket;              // Descrittore del socket di ascolto
struct sockaddr_in listener_addr; // Struttura per gestire il socket di ascolto
socklen_t listener_addr_len;

int tmp;                      // Variabile di servizio
char stdin_buff[MAX_STDIN_C]; // Buffer per i comandi da standard input
char socket_buffer[MAX_ENTRY_UPDATE];

// Identifica il server
int server_port;

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

    // creazione socket di ascolto
    listener_socket = udp_socket_init(&listener_addr, &listener_addr_len, my_port);

    // peer non connesso
    nbs.tot = -1;

    // inizializzo set di descrittori
    FD_SET(listener_socket, &master);
    FD_SET(0, &master);
    fdmax = listener_socket;

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

            fgets(stdin_buff, MAX_STDIN_C, stdin);
            sscanf(stdin_buff, "%s", command);

            // help
            if (strcmp(command, "help") == 0)
            {
                for (tmp = 0; tmp < 5; tmp++)
                    help_client(tmp);
            }

            // start
            else if (strcmp(command, "start") == 0)
            {
                char DS_addr[INET_ADDRSTRLEN]; // Indirizzo IP del server
                char recv_buffer[MAX_LIST_LEN];
                char temp_buffer[MESS_TYPE_LEN];

                printf("Inizio connessione...\n");

                // controllo che il peer non sia connesso
                if (nbs.tot != -1)
                {
                    printf("Il peer e' gia' connesso al DS. Il comando non ha effetto\n");
                    continue;
                }

                // lettura e controllo dei parametri
                tmp = sscanf(stdin_buff, "%s %s %d", command, DS_addr, &server_port);

                if (tmp != 3 || strcmp(DS_addr, LOCALHOST) != 0 || valid_port(server_port))
                {
                    printf("Errore: passaggio dei parametri alla chiamata di start\n");
                    printf("tmp : %d\n", tmp);
                    printf("localhost: %s invece di %s\n", DS_addr, LOCALHOST);
                    printf("port: %d\n", server_port);
                    help_client(1);
                    continue;
                }

                // invio richiesta di connessione
                if (!send_udp_wait_ack(listener_socket, "CONN_REQ", MESS_TYPE_LEN, server_port, "CONN_ACK"))
                {
                    printf("Errore impossibile inviare richiesta di connessione al server. Riprovare\n");
                    continue;
                }

                // ricevo lista di vicini
                if (!recv_udp_and_ack(listener_socket, recv_buffer, MAX_LIST_LEN, server_port, "NBR_LIST", "LIST_ACK"))
                {
                    printf("Errore impossibile ricevere la lista di vicini dal server. Riprovare\n");
                    continue;
                }

                sscanf(recv_buffer, "%s %d %d", temp_buffer, &nbs.prev, &nbs.next);

                // controllo sui parametri letti
                if (!valid_port(nbs.prev) || !valid_port(nbs.next))
                {
                    printf("Errore lista di vicini ricevuta non valida. Riprovare\n");
                    continue;
                }

                if (nbs.prev != nbs.next)
                {
                    nbs.tot = 2;
                }
                else if (nbs.prev != -1)
                {
                    nbs.tot = 1;
                }
                else
                {
                    nbs.tot = 0;
                }
                printf("Connessione riuscita\n");
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
                    if (!send_udp_wait_ack(listener_socket, "CLT_EXIT", MESS_TYPE_LEN, server_port, "C_EX_ACK"))
                    {
                        printf("Errore disconnessione non riuscita\n");
                        continue;
                    }

                    printf("Disconnessione riuscita\n");
                }

                // chiusura socket e terminazione
                close(listener_socket);
                _exit(0);
            }

            else
                printf("Errore, comando non esistente\n");

            FD_CLR(0, &readset);
        }

        // messaggio sul socket
        if (FD_ISSET(listener_socket, &readset))
        {
            int src_port;
            char mess_type_buffer[MESS_TYPE_LEN + 1];

            src_port = s_recv_udp(listener_socket, socket_buffer, MAX_ENTRY_UPDATE);

            sscanf(socket_buffer, "%s", mess_type_buffer);
            mess_type_buffer[MESS_TYPE_LEN] = '\0';

            // server
            if (src_port == server_port)
            {
                printf("Messaggio ricevuto dal server: %s\n", socket_buffer);

                // aggiornamento vicino precedente
                if (strcmp(mess_type_buffer, "PRE_UPDT") == 0)
                {
                    sscanf(socket_buffer, "%s %d", mess_type_buffer, &tmp);

                    if (valid_port(tmp) && s_send_ack_udp(listener_socket, "PREV_ACK", server_port))
                    {
                        printf("Aggiornamento vicino precedente da %d a %d avvenuto con successo", nbs.prev, tmp);
                        nbs.prev = tmp;
                    }

                    printf("Errore nell'aggiornamento del vicino precedente da %d a %d\nOperazione annullata", nbs.prev, tmp);
                }

                // aggiornamento vicino successivo
                else if (strcmp(mess_type_buffer, "NXT_UPDT") == 0)
                {
                    sscanf(socket_buffer, "%s %d", mess_type_buffer, &tmp);

                    if (valid_port(tmp) && s_send_ack_udp(listener_socket, "NEXT_ACK", server_port))
                    {
                        printf("Aggiornamento vicino successivo da %d a %d avvenuto con successo", nbs.next, tmp);
                        nbs.next = tmp;
                    }

                    printf("Errore nell'aggiornamento del vicino successivo da %d a %d\nOperazione annullata", nbs.next, tmp);
                }

                // Notifica chiusura server
                else if (strcmp(mess_type_buffer, "SRV_EXIT") == 0)
                {
                    printf("Il server sta per chiudere\nDisconnessione in corso...\n");

                    // Invia ACK
                    s_send_ack_udp(listener_socket, "S_XT_ACK", server_port);

                    printf("Disconnessione riuscita");
                    // Chiude
                    close(listener_socket);
                    _exit(0);
                }

            }
            FD_CLR(listener_socket, &readset);
        }
    }
    return 0;
}
