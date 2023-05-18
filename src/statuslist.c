#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "list.h"
#include "statuslist.h"

List *status_list;

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
        if (((Status *)current->head)->pid == pid) {
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

void remove_terminated_status() {
    List *current = status_list;
    List *prev = NULL;

    // Traverse the list
    while (current != NULL) {
        if (((Status *)current->head)->status.mode != RUNNING) {
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

void add_status_to_list(Status *s) {
    status_list = list_append(s, status_list);
}

void change_status(pid_t pid, StatusCode status) {
    List *list = status_list;
    while (list != NULL) {
        Status *s = list->head;
        if (s->pid == pid) {
            s->status = status;
            return;
        }
        list = list->tail;
    }
}

void print_status_list() {
    printf("PID\tPGID\tSTATUS\t\tPROGRAM\n");
    List *list = status_list;
    while (list != NULL) {
        Status *s = list->head;
        if (s) {
            if (s->status.mode == RUNNING)
                printf("%d \t %d \t running \t %s \n", s->pid, s->pgid, s->program);
            else if (s->status.mode == EXITED)
                printf("%d \t %d \t exited(%d) \t %s \n", s->pid, s->pgid, s->status.code, s->program);
            else if (s->status.mode == SIGNALED)
                printf("%d \t %d \t signaled(%d) \t %s \n", s->pid, s->pgid, s->status.code, s->program);
        }
        list = list->tail;
    }
}
