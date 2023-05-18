#ifndef STATUSLIST_H

#define STATUSLIST_H

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
void add_status_to_list(Status *s);
void remove_status(pid_t pid);
void remove_terminated_status();
void change_status(pid_t pid, StatusCode status);
void print_status_list();

#endif /* end of include guard: STATUSLIST_H */
