#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "list.h"
#include "statuslist.h"

List *status_list;

void add_status(int pid, int pgid, char *status, char *program) {
    Status *new_status = malloc(sizeof(Status));
    new_status->pid = pid;
    new_status->pgid = pgid;
    new_status->status = status;
    new_status->program = program;

    List *list = status_list;
    while (list != NULL) {
        list = list->tail;
    }
    status_list = list_append(new_status, status_list);
}

void change_status(int pid, char *status) {
    List *list = status_list;
    while (list != NULL) {
        Status *s = list->head;
        if (s->pid == pid) {
            s->status = status;
            break;
        }
        list = list->tail;
    }
}

void print_status_list() {
    printf("PID\tPGID\tSTATUS\tPROGRAM\n");
    List *list = status_list;
    while (list != NULL) {
        Status *s = list->head;
        printf("%d\t%d\t%s\t%s\n", s->pid, s->pgid, s->status, s->program);
        list = list->tail;
    }
}


