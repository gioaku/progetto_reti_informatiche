#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "util_s.h"

void print_server_commands()
{
    printf("Elenco dei comandi disponibili:\n");
    printf("$ help                   - mostra elenco comandi e significato\n");
    printf("$ showpeers              - mostra elenco peer connessi alla rete\n");
    printf("$ showneighbors [<peer>] - mostra i neighbor di un peer o la lista completa se peer viene omesso\n");
    printf("$ esc                    - termina il DS\n");
}

struct Date get_start_date(struct Date today)
{

    FILE *fd;
    if (!file_exists(START_DATE_FILE))
    {
        create_path(START_DATE_DIR);
        fd = fopen(START_DATE_FILE, "w");
        fprintf(fd, DATE_FORMAT, today.y, today.m, today.d);
        fclose(fd);
        return today;
    }
    else
    {
        struct Date ret;
        fd = fopen(START_DATE_FILE, "r");
        fscanf(fd, DATE_FORMAT, &ret.y, &ret.m, &ret.d);
        fclose(fd);
        return ret;
    }
}

void send_updated_date(int sig)
{
    extern struct Date today;
    extern struct UdpSocket sock;
    update_date(&today);

    // lunghezza del messaggio da inviare
    int msg_len;
    // buffer per la data da inviare
    char date_buffer[DATE_LEN];

    //composizione messaggio
    dtoa(date_buffer, today);
    msg_len = sprintf(sock.buffer, "%s %s", "SET_TDAY", date_buffer);
    sock.buffer[msg_len] = '\0';
    printf("Lista da inviare ai peer: %s (lunga %d byte)\n", sock.buffer, msg_len);

    int i;
    for (i = 0; i < get_n_peers(); i++)
    {
        if (!send_udp_wait_ack(sock.id, sock.buffer, msg_len, get_port(i), "TDAY_ACK"))
        {
            printf("Errore: impossibile comunicare data al peer %d\n", get_port(i));
        }
    }
}
