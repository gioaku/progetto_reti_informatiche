int valid_port(int port){
    return (port >= 0 && port <= 65536);
}
void print_error(char* message){
    printf("Errore: %s\n", message);
}