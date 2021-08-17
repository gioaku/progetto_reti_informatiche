#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#define __USE_XOPEN
#include <time.h>

#include "util_c.h"

void print_client_commands()
{
    printf("Elenco dei comandi disponibili:\n");
    printf("$ help                                - mostra elenco comandi e significato\n");
    printf("$ start <DS_addr> <DS_port>           - richiede al Discovery Server connessione alla rete\n");
    printf("$ add <type> <quantity>               - aggiunge una entry nel registro del peer\n");
    printf("$ get <aggr> <type> [<date1>-<date2>] - richiede un dato aggregato\n");
    printf("$ stop                                - richiede disconnessione dalla rete\n");
    return;
}

void help_client(int i)
{
    switch (i)
    {
    case 0: // help
        printf("\nhelp - mostra elenco comandi e significato\n");
        printf("Parametri: nessuno\n");
        break;
    case 1: // start
        printf("\nstart <DS_addr> <DS_port> - richiede al Discovery Server connessione alla rete\n");
        printf("Parametri:\n");
        printf(" - DS_addr : stringa con IP in formato presentazione\n");
        printf(" - DS_port : porta sulla quale gira il processo server, compresa tra 1024 e 65535\n");
        break;
    case 2: // add
        printf("\nadd <type> <quantity> - aggiunge una entry nel registro giornaliero del peer\n");
        printf("Parametri:\n");
        printf(" - type : carattere per indicare se tamponi o nuovi casi [t/n]\n");
        printf(" - quantity : quantita' del dato da aggiungere [intero n]\n");
        break;
    case 3: // get
        printf("\nget <aggr> <type> [<date1>-<date2>] - richiede un dato aggregato\n");
        printf("Parametri:\n");
        printf(" - aggr : tipo di aggregazione, se totale o variazioni giornaliere [t/v]\n");
        printf(" - type : carattere per indicare se tamponi o nuovi casi [t/n]\n");
        printf(" - date1 : lower bound della ricerca [dd:mm:yyyy], '*' per lower bound minimo\n");
        printf(" - date2 : upper bound della ricerca [dd:mm:yyyy], '*' per upper bound massimo\n");
        printf("    * se non si vuole nessun bound omettere le date\n");
        printf("    *il periodo non deve superare la durata di un anno\n");
        break;
    case 4:
        printf("\nstop - richiede disconnessione dalla rete\n");
        printf("Parametri: nessuno\n");
        break;
    default:
        printf("Errore: stato non previsto\n");
    }
    return;
}

int get_path_string(char *path, int port, char type, char *dir)
{
    int len;
    len = sprintf(path, PATH_FORMAT, REGISTERS, port, type, dir);
    path[len] = '\0';
    return len;
}

int get_file_string(char *file, int port, char type, char *dir, struct Date date)
{
    int len;
    len = sprintf(file, FILE_FORMAT, REGISTERS, port, type, dir, date.y, date.m, date.d);
    file[len] = '\0';
    return len;
}

int get_filename_string(char *filename, struct Date date)
{
    int len;
    len = sprintf(filename, FILENAME_FORMAT, date.y, date.m, date.d);
    filename[len] = '\0';
    return len;
}

void append_entry(int port, struct Date date, char type, int qty)
{
    FILE *fd;
    char path[MAX_FILE_LEN + 1];
    char filename[FILENAME_LEN + 1];

    get_path_string(path, port, type, ENTRIES);
    get_filename_string(filename, date);

    printf("Ricerca del file '%s%s' in corso...\n", path, filename);

    if (!file_exists(path))
    {
        create_path(path);
    }

    fd = fopen(strcat(path, filename), "a");
    fprintf(fd, "%d\n", qty);
    fclose(fd);

    printf("Entry %d aggiunta al file '%s'\n", qty, path);
}

FILE *open_reg(int port, char type, struct Date date, char *mode)
{
    char path[MAX_FILE_LEN + 1];
    char filename[FILENAME_LEN + 1];

    get_path_string(path, port, type, ENTRIES);
    get_filename_string(filename, date);

    printf("File: <open_reg> %s%s\n", path, filename);

    if (!file_exists(path))
    {
        create_path(path);
    }

    return fopen(strcat(path, filename), mode);
}

int check_period(char *period, struct Date start_date, struct Date today, struct Date *from, struct Date *to)
{

    char *date1, *date2;
    char *ptr;
    int ret;
    struct Date tmp;

    ptr = strchr(period, '-');

    if (ptr == NULL)
    {
        return 0;
    }

    (*ptr) = '\0';
    date1 = period;
    date2 = ptr + 1;

    // controllo *-* non valido
    if (strcmp(date1, "*") == 0 && strcmp(date2, "*") == 0)
    {
        return 0;
    }

    // assegnazione from e to

    // se from unbounded
    if (strcmp(date1, "*") == 0)
    {
        // assegnazione to
        ret = sscanf(date2, DATE_IN_FORMAT, &to->d, &to->m, &to->y);
        if (ret != 3 || !dvalid(*to) || !soonereq(*from, *to) || !sooner(*to, today))
        {
            (*ptr) = '-';
            return 0;
        }

        // tmp = un anno prima di to
        tmp.d = to->d;
        tmp.m = to->m;
        tmp.y = to->y - 1;
        dnext(&tmp);

        // assegnazione from
        if (sooner(start_date, tmp))
        {
            from->d = tmp.d;
            from->m = tmp.m;
            from->y = tmp.y;
        }
        else
        {
            from->d = start_date.d;
            from->m = start_date.m;
            from->y = start_date.y;
        }
    }
    // from definito
    else
    {
        //assegnazione from
        ret = sscanf(date1, DATE_IN_FORMAT, &from->d, &from->m, &from->y);
        if (ret != 3 || !dvalid(*from) || !soonereq(start_date, *from) || !sooner(*from, today))
        {
            (*ptr) = '-';
            return 0;
        }

        // to unbounded
        if (strcmp(date2, "*") == 0)
        {
            // tmp = un anno dopo di from
            tmp.d = from->d;
            tmp.m = from->m;
            tmp.y = from->y + 1;
            dprev(&tmp);

            // assegnazione to
            if (sooner(tmp, today))
            {
                to->d = tmp.d;
                to->m = tmp.m;
                to->y = tmp.y;
            }
            else
            {
                to->d = today.d;
                to->m = today.m;
                to->y = today.y;
                dprev(to);
            }
        }

        // to definito
        else
        {
            // assegnazione to
            ret = sscanf(date2, DATE_IN_FORMAT, &to->d, &to->m, &to->y);
            if (ret != 3 || !dvalid(*to) || !soonereq(*from, *to) || !sooner(*to, today))
            {
                (*ptr) = '-';
                return 0;
            }

            // tmp = un anno prima di to
            tmp.d = to->d;
            tmp.m = to->m;
            tmp.y = to->y - 1;
            dnext(&tmp);

            // se periodo troppo lungo errore
            if (sooner(*from, tmp))
            {
                return 0;
            }
        }
    }
    (*ptr) = '-';
    return 1;
}

int get_total(int udp, int port, char type, struct Date date, struct Neighbors nbs, int server_port)
{
    // valore di ritorno
    int ret;
    // socket tcp per comunicare con i vicini
    int sock;
    int msg_len;
    // buffer per la comunicazione
    char buffer[MAX_TCP_MSG + 1], header_buff[HEADER_LEN + 1];
    // porta ricevuta
    int peer_port;

    // se dato salvato lo ritorno
    ret = get_saved_elab(port, type, date);
    if (ret != -1)
    {
        return ret;
    }
    // se non ci sono altri ritorno la somma dei miei e la salvo
    if (nbs.tot == 0)
    {
        ret = get_entries_sum(port, type, date);
        create_elab(port, type, date, ret);
        return ret;
    }

    // chiedo ai miei vicini se hanno il dato

    sock = tcp_connect_init(nbs.prev);

    msg_len = sprintf(buffer, "ELAB_REQ %c %04d_%02d_%02d", type, date.y, date.m, date.d);
    buffer[msg_len] = '\0';
    send_tcp(sock, buffer, msg_len);

    recv_tcp(sock, buffer);

    close(sock);

    msg_len = sscanf(buffer, "%s %d", header_buff, &ret);
    if (msg_len == 2 && strcmp(header_buff, "ELAB_ACK") == 0 && ret != -1)
    {
        char file[MAX_FILE_LEN + 1];
        get_file_string(file, port, type, ENTRIES, date);
        remove(file);
        create_elab(port, type, date, ret);
        return ret;
    }

    if (nbs.tot > 1)
    /*
    {
        FILE *fd;
        int qty;

        sock = tcp_connect_init(nbs.prev);
        msg_len = sprintf(buffer, "SEND_ALL %c %04d_%02d_%02d", type, date.y, date.m, date.d);
        send_tcp(sock, buffer, msg_len);
        fd = open_reg(port, type, date, "a");

        while (recv_tcp(sock, buffer) > 0)
        {

            msg_len = sscanf(buffer, "%s %d", header_buff, &qty);
            header_buff[HEADER_LEN] = '\0';

            if (msg_len == 2 && strcmp("NW_ENTRY", header_buff) == 0)
            {
                send_tcp(sock, "NW_E_ACK", HEADER_LEN);

                fprintf(fd, "%d\n", qty);
                ret += qty;
            }
        }

        create_elab(port, type, date, ret);
        return ret;
    }
*/
    {
        sock = tcp_connect_init(nbs.next);

        msg_len = sprintf(buffer, "ELAB_REQ %c %04d_%02d_%02d", type, date.y, date.m, date.d);

        send_tcp(sock, buffer, msg_len);

        recv_tcp(sock, buffer);
        close(sock);

        msg_len = sscanf(buffer, "%s %d", header_buff, &ret);
        if (msg_len == 2 && strcmp(header_buff, "ELAB_ACK") == 0 && ret != -1)
        {
            char file[MAX_FILE_LEN + 1];
            get_file_string(file, port, type, ENTRIES, date);
            remove(file);
            create_elab(port, type, date, ret);
            return ret;
        }
    }
    // altirmenti provo a cercare qualcuno che abbia tutti i dati
    if (server_port != -1)
    {
        send_udp_wait_ack(udp, "LOCK_REQ", HEADER_LEN, server_port, "LOCK_ACK");
    }

    msg_len = sprintf(buffer, "FL_A_REQ %d %c %04d_%02d_%02d", port, type, date.y, date.m, date.d);
    buffer[msg_len] = '\0';

    send_udp_wait_ack(udp, buffer, msg_len, nbs.next, "FL_A_ACK");

    recv_udp_and_ack(udp, buffer, MAX_UDP_MSG, ALL_PORT, "PROP_ALL", "PR_A_ACK");
    msg_len = sscanf(buffer, "%s %d", header_buff, &peer_port);
    buffer[msg_len] = '\0';

    if (peer_port)
    {
        int qty;
        FILE *fd;

        sock = tcp_connect_init(peer_port);
        msg_len = sprintf(buffer, "SEND_ALL %c %04d_%02d_%02d", type, date.y, date.m, date.d);
        send_tcp(sock, buffer, msg_len);

        fd = open_reg(port, type, date, "w");
        ret = 0;

        while (recv_tcp(sock, buffer) > 0)
        {

            msg_len = sscanf(buffer, "%s %d", header_buff, &qty);
            header_buff[HEADER_LEN] = '\0';

            if (msg_len == 2 && strcmp("NW_ENTRY", header_buff) == 0)
            {
                send_tcp(sock, "NW_E_ACK", HEADER_LEN);

                fprintf(fd, "%d\n", qty);
                ret += qty;
            }
        }

        create_elab(port, type, date, ret);
        return ret;
    }

    // bisogna raccogliere tutte le entries

    msg_len = sprintf(buffer, "FL_S_REQ %d %c %04d_%02d_%02d", port, type, date.y, date.m, date.d);
    buffer[msg_len] = '\0';
    send_udp_wait_ack(udp, buffer, msg_len, nbs.next, "FL_S_ACK");

    ret = collect_all_entries(port, udp, type, date);

    if (server_port != -1)
    {
        send_udp_wait_ack(udp, "UNLK_REQ", HEADER_LEN, server_port, "UNLK_ACK");
    }

    return ret;
}

int get_saved_elab(int port, char type, struct Date date)
{
    char file[MAX_FILE_LEN + 1];
    int ret;

    get_file_string(file, port, type, ELABS, date);

    if (file_exists(file))
    {
        FILE *fd;

        fd = fopen(file, "r");
        fscanf(fd, "%d", &ret);
        fclose(fd);
        return ret;
    }
    return -1;
}

int get_entries_sum(int port, char type, struct Date date)
{
    FILE *fd;
    char file[MAX_FILE_LEN + 1];
    int qty, tot;

    get_file_string(file, port, type, ENTRIES, date);

    printf("File: <get_entries_sum> %s\n", file);

    if (!file_exists(file))
    {
        return 0;
    }

    tot = 0;
    fd = fopen(file, "r");

    while (fscanf(fd, "%d\n", &qty) != EOF)
    {
        tot += qty;
    }

    fclose(fd);
    printf("Somma entries nel file '%s': %d\n", file, tot);
    return tot;
}

void create_elab(int port, char type, struct Date date, int qty)
{
    FILE *fd;
    char path[MAX_FILE_LEN + 1];
    char filename[FILENAME_LEN + 1];

    get_path_string(path, port, type, ELABS);
    get_filename_string(filename, date);

    printf("File: <create_elab> %s%s, qty:%d\n", path, filename, qty);

    if (!file_exists(path))
    {
        create_path(path);
    }

    fd = fopen(strcat(path, filename), "w");
    fprintf(fd, "%d\n", qty);
    fclose(fd);

    printf("Elab %d inserita nel file: %s\n", qty, path);
}

void handle_tcp_socket(int port, int sock)
{
    char buffer[MAX_TCP_MSG + 1];
    char header_buff[HEADER_LEN + 1];
    int ret;

    ret = recv_tcp(sock, buffer);
    buffer[ret] = '\0';
    strncpy(header_buff, buffer, HEADER_LEN);
    header_buff[HEADER_LEN] = '\0';
    while (ret > 0)
    {
        if (strcmp(header_buff, "ELAB_REQ") == 0)
        {
            char type;
            int msg_len;
            struct Date date;
            ret = sscanf(buffer, "%s %c %04d_%02d_%02d", header_buff, &type, &date.y, &date.m, &date.d);
            if (ret != 5)
            {
                return;
            }

            msg_len = sprintf(buffer, "ELAB_ACK %d", get_saved_elab(port, type, date));
            buffer[msg_len] = '\0';

            send_tcp(sock, buffer, msg_len);
        }

        else if (strcmp(header_buff, "SEND_ALL") == 0)
        {
            char type;
            struct Date date;
            char file[MAX_FILE_LEN + 1];

            sscanf(buffer, "%s %c %04d_%02d_%02d", header_buff, &type, &date.y, &date.m, &date.d);

            get_file_string(file, port, type, ENTRIES, date);

            if (!file_exists(file))
            {
                printf("Errore: richiesta di dati non posseduti, file '%s' non esistente\n", file);
                return;
            }
            send_all_entries_from_file(file, sock);

            return;
        }
        else if (strcmp(header_buff, "EXIT_PRV") == 0)
        {
            char type;
            struct Date date;
            char file[MAX_FILE_LEN + 1];
            int save_elab = 0;
            int sum = 0;

            send_tcp(sock, "EX_P_ACK", HEADER_LEN);

            while (recv_tcp(sock, buffer))
            {
                strncpy(header_buff, buffer, HEADER_LEN);
                header_buff[HEADER_LEN] = '\0';
                // se recv_all
                if (strcmp(header_buff, "RECV_ALL") == 0)
                {
                    if (save_elab)
                    {
                        create_elab(port, type, date, sum);
                    }

                    sscanf(buffer, "%s %c %04d_%02d_%02d", header_buff, &type, &date.y, &date.m, &date.d);

                    get_file_string(file, port, type, ENTRIES, date);
                    if (get_saved_elab(port, type, date) == -1)
                    {
                        send_tcp(sock, "RECV_RDY", HEADER_LEN);
                        remove(file);
                        save_elab = 1;
                        sum = 0;
                    }
                    else
                    {
                        send_tcp(sock, "RECV_SKP", HEADER_LEN);
                    }
                }

                // se recv_sme
                else if (strcmp(header_buff, "RECV_SME") == 0)
                {
                    if (save_elab)
                    {
                        create_elab(port, type, date, sum);
                    }

                    sscanf(buffer, "%s %c %04d_%02d_%02d", header_buff, &type, &date.y, &date.m, &date.d);

                    get_file_string(file, port, type, ENTRIES, date);
                    if (get_saved_elab(port, type, date) == -1)
                    {
                        send_tcp(sock, "RECV_RDY", HEADER_LEN);
                        save_elab = 0;
                    }
                    else
                    {
                        send_tcp(sock, "RECV_SKP", HEADER_LEN);
                    }
                }

                // se nw_entry
                else if (strcmp(header_buff, "NW_ENTRY") == 0)
                {
                    int qty;

                    sscanf(buffer, "%s %d", header_buff, &qty);
                    append_entry(port, date, type, qty);
                    if (save_elab)
                    {
                        sum += qty;
                    }
                    send_tcp(sock, "NW_E_ACK", HEADER_LEN + 1);
                }
            }

            if (save_elab)
            {
                create_elab(port, type, date, sum);
            }
        }
        ret = recv_tcp(sock, buffer);
    }
}

int collect_all_entries(int port, int udp, char type, struct Date date)
{
    int tot;
    int recv_port;
    char header_buff[HEADER_LEN + 1];
    char buffer[MAX_UDP_MSG + 1];
    int msg_len;
    int sock;
    fd_set readset;
    int qty;

    tot = get_entries_sum(port, type, date);

    FD_ZERO(&readset);
    FD_SET(udp, &readset);

    while (1)
    {
        select(udp + 1, &readset, NULL, NULL, NULL);

        if (FD_ISSET(udp, &readset))
        {
            recv_port = s_recv_udp(udp, buffer, MAX_UDP_MSG);
            sscanf(buffer, "%s", header_buff);
            header_buff[HEADER_LEN] = '\0';

            if (strcmp(header_buff, "PROP_SME") == 0)
            {
                sscanf(buffer, "%s %d", header_buff, &recv_port);
                printf("UDP: ricevuto messaggio '%s' dal mittente %d\n", buffer, recv_port);

                if (valid_port(recv_port))
                {
                    sock = tcp_connect_init(recv_port);
                    msg_len = sprintf(buffer, "SEND_ALL %c %04d_%02d_%02d", type, date.y, date.m, date.d);

                    send_tcp(sock, buffer, msg_len);

                    while (recv_tcp(sock, buffer) > 0)
                    {
                        msg_len = sscanf(buffer, "%s %d", header_buff, &qty);
                        header_buff[HEADER_LEN] = '\0';
                        if (msg_len == 2 && (strcmp(header_buff, "NW_ENTRY") == 0))
                        {
                            send_tcp(sock, "NW_E_ACK", HEADER_LEN + 1);
                            printf("Ricevuta nuova entry %d\n", qty);
                            append_entry(port, date, type, qty);
                            tot += qty;
                        }
                    }
                }
            }
            else if (strcmp(header_buff, "FL_S_REQ") == 0)
            {
                printf("UDP: ritornato messaggio '%s'\n", buffer);
                send_ack_udp(udp, "FL_S_ACK", recv_port);

                create_elab(port, type, date, tot);
                return tot;
            }
        }
    }
}

void send_entries_to_next(int port, int next, struct Date start_date, struct Date today)
{
    struct Date date;
    int sock;
    char buffer[MAX_TCP_MSG + 1];
    int msg_len;
    char file[MAX_FILE_LEN + 1];
    char type;

    sock = tcp_connect_init(next);
    send_tcp(sock, "EXIT_PRV", HEADER_LEN);
    recv_tcp(sock, buffer);
    buffer[HEADER_LEN] = '\0';

    if (strcmp(buffer, "EX_P_ACK") != 0)
    {
        printf("Errore: impossibile inviare le entries a next\n");
        return;
    }

    // per ogni giorno valido
    for (date = start_date; sooner(date, today); dnext(&date))
    {
        // nuovi casi
        type = 'n';

        // se ho tutti i nuovi casi mando una RECV_ALL
        if (get_saved_elab(port, type, date) != -1 && get_entries_sum(port, type, date))
        {
            msg_len = sprintf(buffer, "RECV_ALL %c %04d_%02d_%02d", type, date.y, date.m, date.d);

            send_tcp(sock, buffer, msg_len);

            recv_tcp(sock, buffer);
            buffer[HEADER_LEN] = '\0';
            if (strcmp(buffer, "RECV_RDY") == 0)
            {
                get_file_string(file, port, type, ENTRIES, date);
                send_all_entries_from_file(file, sock);
            }
        }

        // se ho alcuni nuovi casi mando una RECV_SME
        else if (get_entries_sum(port, type, date))
        {

            msg_len = sprintf(buffer, "RECV_SME %c %04d_%02d_%02d", type, date.y, date.m, date.d);

            send_tcp(sock, buffer, msg_len);

            recv_tcp(sock, buffer);
            buffer[HEADER_LEN] = '\0';
            if (strcmp(buffer, "RECV_RDY") == 0)
            {
                get_file_string(file, port, type, ENTRIES, date);
                send_all_entries_from_file(file, sock);
            }
        }

        // tamponi
        type = 't';

        // se ho tutti i nuovi tamponi mando una RECV_ALL
        if (get_saved_elab(port, type, date) != -1 && get_entries_sum(port, type, date))
        {
            msg_len = sprintf(buffer, "RECV_ALL %c %04d_%02d_%02d", type, date.y, date.m, date.d);

            send_tcp(sock, buffer, msg_len);

            recv_tcp(sock, buffer);
            buffer[HEADER_LEN] = '\0';
            if (strcmp(buffer, "RECV_RDY") == 0)
            {
                get_file_string(file, port, type, ENTRIES, date);
                send_all_entries_from_file(file, sock);
            }
        }

        // se ho alcuni nuovi tamponi mando una RECV_SME
        else if (get_entries_sum(port, type, date))
        {
            msg_len = sprintf(buffer, "RECV_SME %c %04d_%02d_%02d", type, date.y, date.m, date.d);

            send_tcp(sock, buffer, msg_len);

            recv_tcp(sock, buffer);
            buffer[HEADER_LEN] = '\0';
            if (strcmp(buffer, "RECV_RDY") == 0)
            {
                get_file_string(file, port, type, ENTRIES, date);
                send_all_entries_from_file(file, sock);
            }
        }
    }

    close(sock);
}

void send_all_entries_from_file(char *file, int sock)
{
    int qty, msg_len;
    char buffer[MAX_TCP_MSG + 1];
    FILE *fd;

    fd = fopen(file, "r");

    while (fscanf(fd, "%d\n", &qty) != EOF)
    {
        msg_len = sprintf(buffer, "NW_ENTRY %d", qty);
        buffer[msg_len] = '\0';
        send_tcp(sock, buffer, msg_len + 1);

        recv_tcp(sock, buffer);
    }
    fclose(fd);
}