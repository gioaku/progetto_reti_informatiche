#include "util.h"
#include "msg.h"

void print_client_commands();
void help_client(int);
void append_entry(char *, char, int);
void create_elab(char, int, int, int, int);
int get_entries_sum(char, int, int, int);
FILE *open_reg_w(char, int, int, int);
int check_period(char *, char *, char *, struct tm *, struct tm *);
void no_period(char *, char *, struct tm *, struct tm *);
int get_saved_elab(char, int, int, int);
int handle_tcp_socket(int);
int collect_entries_waiting_flood(int, char, int, int, int);
