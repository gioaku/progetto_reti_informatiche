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

extern int my_port;
extern int server_port;

// Elenco dei comandi disponibili lato client
void print_client_commands()
{
    printf("Elenco dei comandi disponibili:\n");
    printf("$ help                                - mostra elenco comandi e significato\n");
    printf("$ start <DS_addr> <DS_port>           - richiede al Discovery Server connessione alla rete\n");
    printf("$ add <type> <quantity>               - aggiunge una entry nel registro del peer\n");
    printf("$ get <aggr> <type> [<date1>-<date2>] - richiede un dato aggregato\n");
    printf("$ stop                                - richiede disconnessione dalla rete\n");
}

// Ulteriore ausilio per comprendere i comandi
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
    case 2:
        printf("\nadd <type> <quantity> - aggiunge una entry nel registro giornaliero del peer\n");
        printf("Parametri:\n");
        printf(" - type : carattere per indicare se tamponi o nuovi casi [t/n]\n");
        printf(" - quantity : quantita' del dato da aggiungere [intero n]\n");
        break;
    case 3:
        printf("\nget <aggr> <type> [<date1>-<date2>] - richiede un dato aggregato\n");
        printf("Parametri:\n");
        printf(" - aggr : tipo di aggregazione, se totale o variazioni giornaliere [t/v]\n");
        printf(" - type : carattere per indicare se tamponi o nuovi casi [t/n]\n");
        printf(" - date1 : lower bound della ricerca, '*' per lower bound minimo\n");
        printf(" - date2 : upper bound della ricerca, '*' per upper bound massimo\n");
        printf("Nota: se non si vuole nessun bound omettere le date\n");
        break;
    case 4:
        printf("\nstop - richiede disconnessione dalla rete\n");
        printf("Parametri: nessuno\n");
        break;
    default:
        printf("Errore nella chiamata\n");
    }
}

void append_entry(char *date, char type, int qty)
{
    FILE *fd;
    char path[MAX_PATH_LEN + MAX_FILENAME_LEN + 1];
    char filename[MAX_FILENAME_LEN + 1];
    int path_len, filename_len;
    path_len = sprintf(path, PATH_FORMAT, REGISTERS, my_port, type, ENTRIES);
    path[path_len] = '\0';

    filename_len = sprintf(filename, "%s.txt", date);
    filename[filename_len] = '\0';

    printf("File : %s%s\n", path, filename);

    if (!file_exists(path))
    {
        printf("Creating path : %s\n", path);
        create_path(path);
    }

    fd = fopen(strcat(path, filename), "a");
    fprintf(fd, "%d\n", qty);
    fclose(fd);

    printf("Entry %d inserita nel file: %s\n", qty, path);
}

void create_elab(char type, int d, int m, int y, int qty)
{
    FILE *fd;
    char path[MAX_PATH_LEN + MAX_FILENAME_LEN + 1];
    char filename[MAX_FILENAME_LEN + 1];
    int path_len, filename_len;
    path_len = sprintf(path, PATH_FORMAT, REGISTERS, my_port, type, ELABS);
    path[path_len] = '\0';

    filename_len = sprintf(filename, FILENAME_FORMAT, y, m, d);
    filename[filename_len] = '\0';

    printf("File : %s%s\n", path, filename);

    if (!file_exists(path))
    {
        printf("Creating path : %s\n", path);
        create_path(path);
    }

    fd = fopen(strcat(path, filename), "w");
    fprintf(fd, "%d\n", qty);
    fclose(fd);

    printf("Elab %d inserita nel file: %s\n", qty, path);
}

// Ritorna la somma di tutte le entri nel file
int get_entries_sum(char type, int d, int m, int y)
{
    FILE *fd;
    char file[MAX_PATH_LEN + MAX_FILENAME_LEN + 1];
    int file_len;
    int qty, tot;

    file_len = sprintf(file, FILE_FORMAT, REGISTERS, my_port, type, ENTRIES, y, m, d);
    file[file_len] = '\0';

    printf("File : %s\n", file);

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
    return tot;
}

// Apre un registro in scrittura
FILE *open_reg_w(char type, int d, int m, int y)
{
    char path[MAX_PATH_LEN + MAX_FILENAME_LEN + 1];
    char filename[MAX_FILENAME_LEN + 1];
    int path_len, filename_len;
    path_len = sprintf(path, PATH_FORMAT, REGISTERS, my_port, type, ENTRIES);
    path[path_len] = '\0';

    filename_len = sprintf(filename, FILENAME_FORMAT, y, m, d);
    filename[filename_len] = '\0';

    printf("File : %s%s\n", path, filename);

    if (!file_exists(path))
    {
        printf("Creating path : %s\n", path);
        create_path(path);
    }

    return fopen(strcat(path, filename), "w");
}

// Controlla che il periodo fornito sia valido e inizializza from e to
int check_period(char *period, char *first_day, char *today, struct tm *from, struct tm *to)
{

    char date1[DATE_IN_LEN + 1], date2[DATE_IN_LEN + 1];
    int fd, fm, fy, td, tm, ty;

    if (sscanf(period, "%s-%s", date1, date2) != 2)
    {
        return 0;
    }

    // possibilita * -> date 2
    if (strcmp(date1, "*") == 0)
    {

        if (sscanf(date2, DATE_IN_FORMAT, &td, &tm, &ty) != 3)
            return 0;
        if (!valid_date_i(td, tm, ty))
            return 0;
        if (!in_time_interval(td, tm, ty, first_day, today))
            return 0;

        strptime(first_day, "%Y_%m_%d", from);
        strptime(date2, "%d:%m:%Y", to);
        return 1;
    }

    else
    {
        if (sscanf(date1, DATE_IN_FORMAT, &fd, &fm, &fy) != 3)
            return 0;
        if (!valid_date_i(fd, fm, fy))
            return 0;
        if (!in_time_interval(fd, fm, fy, first_day, today))
            return 0;

        if (strcmp(date2, "*") == 0)
        {
            strptime(date1, "%d:%m:%Y", from);
            strptime(today, "%Y_%m_%d", to);
            to->tm_mday--;
            mktime(to);
            return 1;
        }
        else
        {
            if (sscanf(date2, DATE_IN_FORMAT, &td, &tm, &ty) != 3)
                return 0;
            if (!valid_date_i(td, tm, ty))
                return 0;
            if (!in_time_interval(td, tm, ty, first_day, today))
                return 0;
            if (!sooner_or_eq(fd, fm, fy, td, tm, ty))
                return 0;
            strptime(date1, "%d:%m:%Y", from);
            strptime(date2, "%d:%m:%Y", to);
            return 1;
        }
    }
}

// Inizializza from e to quando il periodo manca
void no_period(char *first_day, char *today, struct tm *from, struct tm *to)
{
    strptime(first_day, "%Y_%m_%d", from);
    strptime(today, "%Y_%m_%d", to);
    to->tm_mday--;
    mktime(to);
}

// Restituisce totale di tipo type in data d/m/y - ritorna -1 altrimenti
int get_saved_elab(char type, int d, int m, int y)
{
    char file[MAX_PATH_LEN + MAX_FILENAME_LEN + 1];
    int ret;

    sprintf(file, FILE_FORMAT, REGISTERS, my_port, type, ENTRIES, y, m, d);

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

int handle_tcp_socket(int sock)
{
    char buffer[MAX_TCP_MSG + 1];
    char msg_type_buffer[MESS_TYPE_LEN + 1];
    int ret;

    while (1)
    {
        ret = recv(sock, buffer, MAX_TCP_MSG, 0);

        if (ret == 0)
            return 0;

        buffer[ret] = '\0';

        strncpy(msg_type_buffer, buffer, MESS_TYPE_LEN);
        msg_type_buffer[MESS_TYPE_LEN] = '\0';

        printf("TCP [%d] : Ricevuto messaggio %s\n", sock, msg_type_buffer);
        if (strcmp(msg_type_buffer, "ELAB_REQ"))
        {
            char type;
            int d, m, y, msg_len;

            ret = sscanf(buffer, "%s %c %04d_%02d_%02d", msg_type_buffer, &type, &y, &m, &d);
            if (ret != 5)
                continue;
            msg_len = sprintf(buffer, "ELAB_ACK %d", get_saved_elab(type, d, m, y) != -1);
            buffer[msg_len] = '\0';
            send(sock, buffer, msg_len, 0);
        }
        if (strcmp(msg_type_buffer, "SEND_ALL"))
        {
            char type;
            int d, m, y, msg_len;
            FILE *fd;
            char file[MAX_PATH_LEN + MAX_FILENAME_LEN + 1];
            int file_len;
            int qty;

            ret = sscanf(buffer, "%s %c %04d_%02d_%02d", msg_type_buffer, &type, &y, &m, &d);
            if (ret != 5)
                continue;
    
            file_len = sprintf(file, FILE_FORMAT, REGISTERS, my_port, type, ENTRIES, y, m, d);
            file[file_len] = '\0';

            if (!file_exists(file))
            {
                close(sock);
                return 0;
            }

            fd = fopen(file, "r");
            while (fscanf(fd, "%d\n", &qty) != EOF)
            {
                msg_len = sprintf(buffer, "NW_ENTRY %d", qty);
                buffer[msg_len] = '\0';
                send(sock, buffer, msg_len, 0);
                recv(sock, buffer, MESS_TYPE_LEN, 0);
            }
            close(sock);
            fclose(fd);
            return 0;
        }
    }
}

int collect_entries_waiting_flood(int udp, char type, int d, int m, int y)
{
    int tot;
    int recv_port;
    char msg_type_buffer[MESS_TYPE_LEN + 1];
    char buffer[MAX_UDP_MSG + 1];
    char date[DATE_LEN + 1];
    int msg_len;
    int sock;
    fd_set readset;
    int qty;

    tot = get_entries_sum(type, d, m, y);
    sprintf(date, DATE_FORMAT, y, m, d);
    FD_ZERO(&readset);
    FD_SET(udp, &readset);

    while (1)
    {
        select(udp + 1, &readset, NULL, NULL, NULL);

        if (FD_ISSET(udp, &readset))
        {
            s_recv_udp(udp, buffer, MAX_UDP_MSG);
            sscanf(buffer, "%s %d", msg_type_buffer, &recv_port);
            msg_type_buffer[MESS_TYPE_LEN] = '\0';

            if (strcmp(msg_type_buffer, "PROP_SME") == 0)
            {
                s_send_ack_udp(udp, "PR_S_ACK", MESS_TYPE_LEN);

                if (valid_port(recv_port))
                {
                    sock = tcp_connect_init(recv_port);
                    msg_len = sprintf(buffer, "SEND_ALL %c %04d_%02d_%02d", type, y, m, d);

                    send(sock, buffer, msg_len, 0);
                    while (recv(sock, buffer, MAX_TCP_MSG, 0))
                    {
                        int ret;
                        ret = sscanf(buffer, "%s %d", msg_type_buffer, &qty);
                        msg_type_buffer[MESS_TYPE_LEN] = '\0';
                        if (ret == 2 && strcmp("NW_ENTRY", msg_type_buffer) == 0)
                        {
                            printf("Ricevuta nuova entry %d\n", qty);
                            append_entry(date, type, qty);
                            tot += qty;
                            send(sock, "NW_E_ACK", MESS_TYPE_LEN, 0);
                        }
                    }
                }
            }
            else if (strcmp(msg_type_buffer, "FL_S_REQ") == 0)
            {
                create_elab(type, d, m, y, tot);
            }
        }
    }
}
