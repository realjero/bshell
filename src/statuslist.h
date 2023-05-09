#ifndef STATUSLIST_H

#define STATUSLIST_H

typedef struct {
    int pid;
    int pgid;
    char *status;
    char *program;
} Status;

void add_status(int pid, int pgid, char *status, char *program);
void change_status(int pid, char *status);
void print_status_list();

#endif /* end of include guard: STATUSLIST_H */
