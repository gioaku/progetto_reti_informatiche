int valid_port(int port){
    return (port >= 0 && port <= 65536);
}

int is_between(int x, int a, int b){
    return (a > b) ? (x > a || x < b) : (x > a && x < b);
}