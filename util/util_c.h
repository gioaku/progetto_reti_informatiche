#include "util.h"
#include "msg.h"

void print_client_commands();
void help_client(int);
void append_entry(char *, char, int);
int check_period(char *, char *, char *, struct tm *, struct tm *);
int get_saved_elab(char, int, int, int);
FILE *open_reg_w(char, int, int, int);
int collect_entries_waiting_flood(int, char, int, int, int);

int check_dates(char *, char *, char);
// void insert_entry(char,int);
void insert_entry_string(char *);
int count_entries(char);
int sum_entries(char);
void write_aggr(int, int, char);
int check_aggr(int, char);
void wait_for_entries(int, int, char);
void send_missing_entries(int, char, char *, char *);
void send_double_missing_entries();
int is_today(char *);
int check_g_lock();
int get_lock();
int check_s_lock();
int stop_lock();
void register_daily_tot(char *);
void print_daily_aggr(char *);
int is_flag_up();
void insert_temp(char *);
void print_results(char, char, int, char *, char *);