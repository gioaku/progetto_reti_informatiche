#include "./util/util_c.h"

// Variabili di stato
int my_port;
int server_port;
struct Date today;
struct Date start_date;

// Udp socket
struct UdpSocket udp;

// Buffer stdin
char command_buffer[MAX_STDIN_C];

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
    // lettura porta dagli argomenti
    my_port = atoi(argv[1]);

    // pulizia set
    FD_ZERO(&master);
    FD_ZERO(&readset);

    // peer non connesso
    nbs.tot = -1;
    server_port = -1;

    // creazione socket udp
    if (udp_socket_init(&udp, my_port) == -1)
    {
        printf("Error: cannot create server socket\n");
        exit(0);
    }

    // inizializzo listener non attivo
    listener_s.id = -1;

    // inizializzo set di descrittori
    FD_SET(0, &master);
    FD_SET(udp.id, &master);
    fdmax = udp.id;

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
            // buffer per leggere il comando
            char command[MAX_COMMAND_C];

            // lettura comando
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
                // indirizzo ip serve
                char ds_address[INET_ADDRSTRLEN + 1];
                // buffer per header del messaggio
                char header_buff[HEADER_LEN + 1];
                // variabile di servizio
                int ret;
                // buffer per contenere le date ricevute
                char date_buffer[DATE_LEN + 1];

                printf("Inizio connessione...\n");

                // controllo che il peer non sia connesso
                if (nbs.tot != -1)
                {
                    printf("Errore: peer gia' connesso alla rete. Il comando non ha effetto\n");
                    FD_CLR(0, &readset);
                    continue;
                }

                // lettura e controllo dei parametri
                ret = sscanf(command_buffer, "%s %s %d", command, ds_address, &server_port);

                // controllo parametri
                if (ret != 3 || strcmp(ds_address, LOCALHOST) != 0 || !valid_port(server_port))
                {
                    printf("Errore: passaggio dei parametri per la connessione non validi\n");
                    help_client(1);
                    FD_CLR(0, &readset);
                    continue;
                }

                // apertura listener tcp
                if (tcp_listener_init(&listener_s, my_port) == -1)
                {
                    printf("Errore: Impossibile creare socket di ascolto\n");
                    exit(1);
                }
                printf("Creato socket di ascolto in attesa di connessione dai vicini\n");

                // invio richiesta di connessione
                while (!send_udp_wait_ack(udp.id, "CONN_REQ", HEADER_LEN, server_port, "CONN_ACK"))
                {
                    printf("Errore: impossibile inviare richiesta di connessione al server\nNuovo tentativo...\n");
                }

                // ricezione lista di vicini
                if (!recv_udp_and_ack(udp.id, udp.buffer, MAX_UDP_MSG, server_port, "NBR_LIST", "LIST_ACK"))
                {
                    printf("Errore: impossibile ricevere la lista di vicini dal server. Riprovare\n");
                    send_udp_wait_ack(udp.id, "CLT_EXIT", HEADER_LEN, server_port, "C_EX_ACK");
                    FD_CLR(0, &readset);
                    continue;
                }

                // lettura dei parametri
                ret = sscanf(udp.buffer, "%s %d %d", header_buff, &nbs.prev, &nbs.next);
                if (ret != 1 && ret != 3)
                {
                    printf("Errore: lista dei vicini ricevuta non valida. Riprovare\n");
                    send_udp_wait_ack(udp.id, "CLT_EXIT", HEADER_LEN, server_port, "C_EX_ACK");
                    FD_CLR(0, &readset);
                    continue;
                }
                if (ret == 1)
                {
                    nbs.tot = 0;
                    printf("Nessun vicino ricevuto. Non ci sono altri peer connessi\n");
                }
                if (ret == 3)
                {
                    nbs.tot = (nbs.prev == nbs.next) ? 1 : 2;
                    printf("I vicini ricevuti sono prev: %d, next %d\n", nbs.prev, nbs.next);
                }

                // controllo sui parametri letti
                if (nbs.tot > 0 && (!valid_port(nbs.prev) || !valid_port(nbs.next)))
                {
                    printf("Errore: lista di vicini ricevuta non valida. Riprovare\n");
                    send_udp_wait_ack(udp.id, "CLT_EXIT", HEADER_LEN, server_port, "C_EX_ACK");
                    FD_CLR(0, &readset);
                    nbs.tot = -1;
                    continue;
                }

                // ricezione data odierna
                if (!recv_udp_and_ack(udp.id, udp.buffer, MAX_UDP_MSG, server_port, "SET_TDAY", "TDAY_ACK"))
                {
                    printf("Errore: impossibile ricevere la di oggi dal server. Riprovare\n");
                    send_udp_wait_ack(udp.id, "CLT_EXIT", HEADER_LEN, server_port, "C_EX_ACK");
                    FD_CLR(0, &readset);
                    continue;
                }

                ret = sscanf(udp.buffer, "%s %s", header_buff, date_buffer);
                today = atod(date_buffer);

                // controllo data odierna ricevuta
                if (ret != 2 || !dvalid(today))
                {
                    printf("Errore nella ricezione della data odierna %s\n", date_buffer);
                    send_udp_wait_ack(udp.id, "CLT_EXIT", HEADER_LEN, server_port, "C_EX_ACK");
                    FD_CLR(0, &readset);
                    continue;
                }

                printf("Data di oggi ricevuta dal server : %s\n", date_buffer);

                // ricevo la start date
                if (!recv_udp_and_ack(udp.id, udp.buffer, MAX_UDP_MSG, server_port, "SET_SDAY", "FDAY_ACK"))
                {
                    printf("Errore: impossibile ricevere la data start dal server. Riprovare\n");
                    send_udp_wait_ack(udp.id, "CLT_EXIT", HEADER_LEN, server_port, "C_EX_ACK");
                    FD_CLR(0, &readset);
                    continue;
                }

                ret = sscanf(udp.buffer, "%s %s", header_buff, date_buffer);
                start_date = atod(date_buffer);

                // controllo sulla start date ricevuta
                if (ret != 2 || !dvalid(start_date))
                {
                    printf("Errore nella ricezione della prima data %s\n", date_buffer);
                    send_udp_wait_ack(udp.id, "CLT_EXIT", HEADER_LEN, server_port, "C_EX_ACK");
                    FD_CLR(0, &readset);
                    continue;
                }
                printf("Data di start ricevuta dal server : %s\n", date_buffer);

                printf("Connessione riuscita\n");

                FD_SET(listener_s.id, &master);
                fdmax = fdmax > listener_s.id ? fdmax : listener_s.id;

                // stampa vicini
                print_nbs(my_port, nbs);
            }

            // add
            else if (strcmp(command, "add") == 0)
            {
                // tipo della nuova entry
                char type;
                // quantita della nuova entry
                int qty;
                // contatore
                int ret;

                // se peer non connesso annulla l'operazione
                if (server_port == -1 || nbs.tot == -1)
                {
                    printf("Errore: peer non connesso\nOperazione abortita\n");
                    FD_CLR(0, &readset);
                    continue;
                }

                // lettura parametri
                ret = sscanf(command_buffer, "%s %c %d", command, &type, &qty);
                if (!(type == 't' || type == 'n') || ret != 3 || qty < 1)
                {
                    printf("Errore : dati inseriti non corretti\nOperazione abortita\n");
                    help_client(2);
                    FD_CLR(0, &readset);
                    continue;
                }

                //inserimento dati
                append_entry(my_port, today, type, qty);
                printf("Salvataggio dati inseriti completato: type %c, date: %02d:%02d:%04d, quantity: %d\n", type, today.d, today.m, today.y, qty);
            }

            // get
            else if (strcmp(command, "get") == 0)
            {
                char aggr;
                char type;
                char period[DATE_IN_LEN * 2 + 2];
                int ret;

                struct Date from;
                struct Date to;
                // time_t from_time;
                // struct tm *from;
                // time_t to_time;
                // struct tm *to;

                // from_time = time(NULL);
                // from = gmtime(&from_time);
                // to_time = time(NULL);
                // to = gmtime(&to_time);

                // se peer non connesso non faccio nulla
                if (server_port == -1 || nbs.tot == -1)
                {
                    printf("Errore: peer non connesso\nOperazione abortita\n");
                    FD_CLR(0, &readset);
                    continue;
                }

                ret = sscanf(command_buffer, "%s %c %c %s", command, &aggr, &type, period);

                // controllo numero di parametri
                if (!(ret == 3 || ret == 4))
                {
                    printf("Errore : numero parametri non corretto\n");
                    help_client(3);
                    FD_CLR(0, &readset);
                    continue;
                }

                // controllo su aggr
                if (!(aggr == 't' || aggr == 'v'))
                {
                    printf("Errore nell'inserimento del dato 'aggr'\n");
                    help_client(3);
                    FD_CLR(0, &readset);
                    continue;
                }

                // controllo su type
                if ((type == 't' || type == 'n'))
                {
                    printf("Errore nell'inserimento  del dato 'type'\n");
                    help_client(3);
                    FD_CLR(0, &readset);
                    continue;
                }

                // controllo sulle date
                if (ret == 4)
                {
                    period[DATE_LEN * 2 + 1] = '\0';
                    if (!check_period(period, start_date, today, &from, &to))
                    {
                        help_client(2);
                        FD_CLR(0, &readset);
                        continue;
                    }
                }
                else if (ret == 3)
                {
                    // from = start_date
                    from.d = start_date.d;
                    from.m = start_date.m;
                    from.y = start_date.y;

                    // to = yesterday
                    to.d = today.d;
                    to.m = today.m;
                    to.y = today.y;
                    dprev(&to);
                }

                printf("GET : aggr %c, type %c, between %02d:%02d:%04d and %02d:%02d:%04d\n", aggr, type, from.d, from.m, from.y, to.d, to.m, to.y);

                // se bisogna calcolare il totale
                if (aggr == 't')
                {
                    struct Date date;
                    int sum = 0;

                    // per ogni giorno ottenere e se necessario salvare dato aggregato
                    for (date = from; soonereq(date, to); dnext(&date))
                    {
                        sum += get_total(udp.id, my_port, type, date, nbs);
                    }
                    /*
                    while (from_time < to_time)
                    {
                        // se ha i dati li aggiunge e continua
                        if ((tmp = get_saved_elab(type, from->tm_mday, from->tm_mon + 1, from->tm_year)) != -1)
                        {
                            sum += tmp;
                            from->tm_mday++;
                            from_time = mktime(from);
                            continue;
                        }
                        // se non ha vicini e non ha dati continua
                        if (nbs.tot == 0)
                        {
                            from->tm_mday++;
                            from_time = mktime(from);
                            continue;
                        }
                        // se ha almeno un vicino chiede a lui
                        if (nbs.tot > 0)
                        {
                            char buffer[MAX_TCP_MSG + 1];
                            char header_buff[HEADER_LEN + 1];
                            int msg_len;
                            int sock;
                            int ret;

                            // connect con vicino precedente
                            sock = tcp_connect_init(nbs.prev);
                            msg_len = sprintf(buffer, "ELAB_REQ %c %04d_%02d_%02d", type, from->tm_year, from->tm_mon + 1, from->tm_mday);

                            // send elab req
                            send(sock, buffer, msg_len, 0);
                            recv(sock, buffer, MAX_TCP_MSG, 0);
                            close(sock);

                            ret = sscanf(buffer, "%s %d", header_buff, &tmp);
                            if (ret == 2 && strcmp(header_buff, "ELAB_ACK") == 0 && tmp != -1)
                            {
                                sum += tmp;
                                create_elab(type, from->tm_mday, from->tm_mon + 1, from->tm_year, tmp);
                                from->tm_mday++;
                                from_time = mktime(from);
                                continue;
                            }
                        }
                        // se ha due vicini chiede anche all'altro
                        if (nbs.tot == 2)
                        {
                            char buffer[MAX_TCP_MSG + 1];
                            char header_buff[HEADER_LEN + 1];
                            int msg_len;
                            int sock;
                            int ret;
                            int peer_port;

                            // connect con vicino successivo
                            sock = tcp_connect_init(nbs.next);
                            msg_len = sprintf(buffer, "ELAB_REQ %c %04d_%02d_%02d", type, from->tm_year, from->tm_mon + 1, from->tm_mday);

                            // send elab req
                            send(sock, buffer, msg_len, 0);
                            recv(sock, buffer, MAX_TCP_MSG, 0);
                            close(sock);

                            ret = sscanf(buffer, "%s %d", header_buff, &tmp);
                            header_buff[HEADER_LEN] = '\0';

                            if (ret == 2 && strcmp(header_buff, "ELAB_ACK") == 0 && tmp != -1)
                            {
                                sum += tmp;
                                create_elab(type, from->tm_mday, from->tm_mon + 1, from->tm_year, tmp);
                                from->tm_mday++;
                                from_time = mktime(from);
                                continue;
                            }
                            // se non ho ricevuto risposte positive cerco qualcuno che le ha tutte
                            msg_len = sprintf(udp.buffer, "FL_A_REQ %d %c %04d_%02d_%02d", my_port, type, from->tm_year, from->tm_mon + 1, from->tm_mday);
                            udp.buffer[msg_len] = '\0';
                            send_udp_wait_ack(udp.id, udp.buffer, msg_len, nbs.next, "FL_A_ACK");
                            recv_udp_and_ack(udp.id, udp.buffer, MAX_UDP_MSG, ALL_PORT, "PROP_ALL", "PR_A_ACK");

                            ret = sscanf(udp.buffer, "%s %d", header_buff, &peer_port);
                            if (ret != 2)
                            {
                                printf("Errore nella ricezione della PROP_ALL %d\n", peer_port);
                                continue;
                            }
                            // se peer_port != 0 contiene la porta di un peer che ha tutti i dati
                            if (peer_port)
                            {
                                int qty;
                                FILE *fd;
                                // mi connetto
                                sock = tcp_connect_init(peer_port);
                                msg_len = sprintf(buffer, "SEND_ALL %c %04d_%02d_%02d", type, from->tm_year, from->tm_mon + 1, from->tm_mday);

                                // invio della richiesta di dati
                                send(sock, buffer, msg_len, 0);

                                // apro file per scrivere le entries
                                fd = open_reg_w(type, from->tm_mday, from->tm_mon + 1, from->tm_year);

                                while (recv(sock, buffer, MAX_TCP_MSG, 0))
                                {
                                    ret = sscanf(buffer, "%s %d", header_buff, &qty);
                                    header_buff[HEADER_LEN] = '\0';
                                    if (ret == 2 && strcmp("NW_ENTRY", header_buff) == 0)
                                    {
                                        printf("Ricevuta nuova entry %d\n", qty);
                                        fprintf(fd, "%d\n", qty);
                                        tmp += qty;
                                        send(sock, "NW_E_ACK", HEADER_LEN, 0);
                                    }
                                }
                                create_elab(type, from->tm_mday, from->tm_mon + 1, from->tm_year, tmp);
                                from->tm_mday++;
                                from_time = mktime(from);
                                continue;
                            }
                            // se ancora non ho ricevuto risposte positive cerco tutti quelli che hanno qualcosa
                            msg_len = sprintf(udp.buffer, "FL_S_REQ %d %c %04d_%02d_%02d", my_port, type, from->tm_year, from->tm_mon + 1, from->tm_mday);
                            udp.buffer[msg_len] = '\0';
                            send_udp_wait_ack(udp.id, udp.buffer, msg_len, nbs.next, "FL_S_ACK");

                            tmp = collect_entries_waiting_flood(udp.id, type, from->tm_mday, from->tm_mon + 1, from->tm_year);
                            sum += tmp;
                            create_elab(type, from->tm_mday, from->tm_mon + 1, from->tm_year, tmp);
                            from->tm_mday++;
                            from_time = mktime(from);
                            continue;
                        }

                        from->tm_mday++;
                        from_time = mktime(from);
                    }*/
                    printf("Totale di %c nel periodo %s: %d\n", type, period, sum);
                }
                if (aggr == 'v')
                {
                    int old, new;
                    struct Date date;

                    dprev(&from);
                    old = get_total(udp.id, my_port, type, from, nbs);
                    dnext(&from);

                    for (date = from; soonereq(date, to); dnext(&date))
                    {
                        new = get_total(udp.id, my_port, type, date, nbs);
                        printf("Variazione di %c il giorno %02d:%02d:%04d: %d", type, date.d, date.m, date.y, new - old);
                        old = new;
                    }
                    /*int old, new;

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
                        */
                }
            }

            // stop
            else if (strcmp(command_buffer, "stop") == 0)
            {

                // se connesso disconnettere e inviare le entries a next
                if (nbs.tot != -1)
                {
                    printf("Disconnessione in corso...\n");

                    // invio delle entries
                    // send_entries_to_next();

                    // tentativo di disconnessione
                    if (!send_udp_wait_ack(udp.id, "CLT_EXIT", HEADER_LEN, server_port, "C_EX_ACK"))
                    {
                        printf("Errore disconnessione non riuscita\n");
                        continue;
                    }

                    printf("Disconnessione riuscita\n");
                }

                // chiusura socket e terminazione
                close(listener_s.id);
                close(udp.id);
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
                close(listener_s.id);
                handle_tcp_socket(my_port, new_sd);
                close(new_sd);
                exit(0);
            }

            close(new_sd);
            FD_CLR(listener_s.id, &readset);
        }

        // messaggio sul socket udp
        if (FD_ISSET(udp.id, &readset))
        {
            int src_port;
            char header_buff[HEADER_LEN + 1];

            src_port = s_recv_udp(udp.id, udp.buffer, MAX_UDP_MSG);

            sscanf(udp.buffer, "%s", header_buff);
            header_buff[HEADER_LEN] = '\0';

            // server
            if (src_port == server_port)
            {
                printf("Messaggio ricevuto dal server: %s\n", udp.buffer);

                // aggiornamento vicino precedente
                if (strcmp(header_buff, "PRE_UPDT") == 0)
                {
                    int port;

                    sscanf(udp.buffer, "%s %d", header_buff, &port);

                    if (valid_port(port) && send_ack_udp(udp.id, "PREV_ACK", server_port))
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
                else if (strcmp(header_buff, "NXT_UPDT") == 0)
                {
                    int port;

                    sscanf(udp.buffer, "%s %d", header_buff, &port);

                    if (valid_port(port) && send_ack_udp(udp.id, "NEXT_ACK", server_port))
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
                else if (strcmp(header_buff, "SET_TDAY") == 0)
                {
                    int tmp;
                    // buffer per contenere la data ricevute
                    char date_buffer[DATE_LEN + 1];

                    // legge data
                    tmp = sscanf(udp.buffer, "%s %s", header_buff, date_buffer);

                    if (tmp != 2 || !dvalid(atod(date_buffer)))
                    {
                        printf("Errore nella ricezione della data odierna");
                        FD_CLR(udp.id, &readset);
                        continue;
                    }
                    // invia ack
                    send_ack_udp(udp.id, "TDAY_ACK", server_port);
                    printf("Data ricevuta dal server : %s\n", date_buffer);
                    today = atod(date_buffer);
                }

                // Notifica chiusura server
                else if (strcmp(header_buff, "SRV_EXIT") == 0)
                {
                    printf("Il server sta per chiudere\nDisconnessione in corso...\n");

                    // Invia ACK
                    send_ack_udp(udp.id, "S_XT_ACK", server_port);

                    printf("Disconnessione riuscita\n");

                    // Chiude tutti i socket
                    close(udp.id);
                    _exit(0);
                }
            }

            // altro peer sul socket udp
            else
            {
                printf("Messaggio ricevuto dal %d: %s\n", src_port, udp.buffer);

                // Flood all entries
                if (strcmp(header_buff, "FL_A_REQ") == 0)
                {
                    char type;
                    struct Date date;
                    int ret;
                    int req_port;
                    int msg_len;

                    // manda ack
                    send_ack_udp(udp.id, "FL_A_ACK", src_port);

                    ret = sscanf(udp.buffer, "%s %d %c %04d_%02d_%02d", header_buff, &req_port, &type, &date.y, &date.m, &date.d);
                    if (ret != 6 || !valid_port(req_port) || (type != 't' && type != 'n') || !dvalid(date) || !soonereq(start_date, date) || !sooner(date, today))
                    {
                        printf("Errore nella ricezione dei parametri della FloodAll\n");
                        FD_CLR(udp.id, &readset);
                        continue;
                    }

                    ret = get_saved_elab(my_port, type, date);
                    if (ret != -1)
                    {
                        msg_len = sprintf(udp.buffer, "PROP_ALL %d", my_port);
                        udp.buffer[msg_len] = '\0';
                        send_udp_wait_ack(udp.id, udp.buffer, msg_len, req_port, "PR_A_ACK");
                    }
                    else if (req_port == nbs.next)
                    {
                        msg_len = sprintf(udp.buffer, "PROP_ALL %d", 0);
                        udp.buffer[msg_len] = '\0';
                        send_udp_wait_ack(udp.id, udp.buffer, msg_len, nbs.next, "PR_A_ACK");
                    }
                    else
                    {
                        msg_len = sprintf(udp.buffer, "FL_A_REQ %d %c %04d_%02d_%02d", req_port, type, date.y, date.m, date.d);
                        udp.buffer[msg_len] = '\0';
                        send_udp_wait_ack(udp.id, udp.buffer, msg_len, nbs.next, "FL_A_ACK");
                    }
                }

                // Flood some entries
                else if (strcmp(header_buff, "FL_S_REQ") == 0)
                {
                    char type;
                    struct Date date;
                    int ret;
                    int req_port;
                    int msg_len;
                    char file[MAX_FILE_LEN + 1];

                    // manda ack
                    send_ack_udp(udp.id, "FL_S_ACK", src_port);

                    ret = sscanf(udp.buffer, "%s %d %c %04d_%02d_%02d", header_buff, &req_port, &type, &date.y, &date.m, &date.d);
                    if (ret != 6 || !valid_port(req_port) || (type != 't' && type != 'n') || !dvalid(date) || !soonereq(start_date, date) || !sooner(date, today))
                    {
                        printf("Errore nella ricezione dei parametri della FloodAll\n");
                        FD_CLR(udp.id, &readset);
                        continue;
                    }
                    get_file_string(file, my_port, type, ENTRIES, date);
                    if (file_exists(file))
                    {
                        msg_len = sprintf(udp.buffer, "PROP_SME %d", my_port);
                        udp.buffer[msg_len] = '\0';
                        send_udp_wait_ack(udp.id, udp.buffer, msg_len, req_port, "PR_S_ACK");
                    }

                    msg_len = sprintf(udp.buffer, "FL_S_REQ %d %c %04d_%02d_%02d", req_port, type, date.y, date.m, date.d);
                    udp.buffer[msg_len] = '\0';

                    if (req_port == nbs.next)
                    {
                        s_send_udp(udp.id, udp.buffer, msg_len, nbs.next);
                    }
                    else
                    {
                        send_udp_wait_ack(udp.id, udp.buffer, msg_len, nbs.next, "FL_S_ACK");
                    }
                }
            }
            FD_CLR(udp.id, &readset);
        }
    }
    return 0;
}
