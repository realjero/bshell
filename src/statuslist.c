#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "statuslist.h"

static List *status_list;

StatusCode term;
bool term_set;


Status *new_status(pid_t pid, pid_t pgid, char *program) {
    Status *new_status = malloc(sizeof(Status));
    new_status->pid = pid;
    new_status->pgid = pgid;
    new_status->status = new_status_code(RUNNING, 0);
    new_status->program = program;
    return new_status;
}

StatusCode new_status_code(MODE mode, int code) {
    StatusCode statusCode = {
            .mode = mode,
            .code = code,
    };
    return statusCode;
}

void remove_status(pid_t pid) {
    List *current = status_list;
    List *prev = NULL;

    // Traverse the list to find the element
    while (current != NULL) {
        if (((Status *) current->head)->pid == pid) {
            // Found the element, remove it from the list
            if (prev != NULL) {
                prev->tail = current->tail;
            } else {
                // The element is the head of the list
                status_list = current->tail;
            }

            free(current);
            return;
        }

        prev = current;
        current = current->tail;
    }

    // Element not found in the list
    printf("Element not found in the list.\n");
}

void remove_terminated_status(void) {
    List *current = status_list;
    List *prev = NULL;

    // Traverse the list
    while (current != NULL) {
        if (((Status *) current->head)->status.mode != RUNNING) {
            // Remove the element that meets the condition
            if (prev != NULL) {
                prev->tail = current->tail;
            } else {
                // The element is the head of the list
                status_list = current->tail;
            }

            List *temp = current;
            current = current->tail;
            free(temp);
        } else {
            prev = current;
            current = current->tail;
        }
    }
}

Status *add_status_to_list(const Status *s) {
    Status *s1 = malloc(sizeof(Status));
    if (s1 == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(EXIT_FAILURE);
    }
    *s1 = *s;
    status_list = list_append(s1, status_list);
    return s1;
}

void add_new_status_to_list(int pid, int pgid, char *command) {
    char *cmd = malloc(sizeof(char) * strlen(command) + 1);
    strcpy(cmd, command);
    // Die Shell schreibt die PID und PGID des neuen Prozesses auf die
    // Statusliste.
    Status s = {
            .pid = pid,
            .pgid = pgid,
            .status = {
                    .mode = RUNNING,
                    .code = 0
            },
            .program = cmd
    };

    Status *new_status = add_status_to_list(&s);

    // child process already terminated; use status saved by sigchld_handler
    if (term_set) {
        new_status->status = term;
        term_set = false;
    }
}

void change_status(pid_t pid, int code) {
    StatusCode statusCode;
    if (WIFEXITED(code)) {
        statusCode = new_status_code(EXITED, WEXITSTATUS(code));
    } else if (WIFSIGNALED(code)) {
        statusCode = new_status_code(SIGNALED, WTERMSIG(code));
    }

    List *list = status_list;
    while (list != NULL) {
        Status *s = list->head;
        if (s->pid == pid) {
            s->status = statusCode;
            return;
        }
        list = list->tail;
    }

    // This status is not yet in the list; save it in the global variable for usage in parent; child might be finished before the process has been added
    term = statusCode;
    term_set = true;
}

void print_status_list(void) {
    printf("PID\tPGID\tSTATUS\t\tPROGRAM\n");
    List *list = status_list;
    while (list != NULL) {
        Status *s = list->head;
        if (s) {
            if (s->status.mode == RUNNING)
                printf("%d \t %d \t running \t %s \n", s->pid, s->pgid, s->program);
            else if (s->status.mode == EXITED)
                printf("%d \t %d \t exit(%d)   \t %s \n", s->pid, s->pgid, s->status.code, s->program);
            else if (s->status.mode == SIGNALED)
                printf("%d \t %d \t signal(%d)   \t %s \n", s->pid, s->pgid, s->status.code, s->program);
        }
        list = list->tail;
    }
}
