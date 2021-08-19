#include "util.h"

int valid_port(int port)
{
    return (port > 0 && port <= 65536);
}

int file_exists(char *file)
{
    struct stat buff;
    return (stat(file, &buff) == 0);
}

int create_path(char *path)
{
    char command[BUFF_SIZE];
    printf("Creazione del path '%s' in corso...\n", path);
    sprintf(command, "mkdir -p %s", path);
    return system(command);
}
