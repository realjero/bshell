#ifndef STATUSLIST_H

#define STATUSLIST_H


#include <stdbool.h>

typedef enum {
    RUNNING,
    EXITED,
    SIGNALED,
} MODE;

typedef struct {
    MODE mode;
    int code;
} StatusCode;

typedef struct {
    pid_t pid;
    int pgid;
    StatusCode status;
    char *program;
} Status;

Status *new_status(pid_t pid, pid_t pgid, char *program);

StatusCode new_status_code(MODE mode, int code);

void add_new_status_to_list(int pid, int pgid, char *command);

void remove_status(pid_t pid);

void remove_terminated_status();

void change_status(pid_t pid, int code);

void print_status_list();

extern StatusCode term;
extern bool term_set;

#endif /* end of include guard: STATUSLIST_H */
