#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "shell.h"
#include "helper.h"
#include "command.h"
#include <signal.h>
#include <errno.h>
#include <pwd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "statuslist.h"
#include "debug.h"
#include "execute.h"

/* do not modify this */
#ifndef NOLIBREADLINE

#include <readline/history.h>

#endif /* NOLIBREADLINE */

extern int shell_pid;
extern int fdtty;

/* do not modify this */
#ifndef NOLIBREADLINE

static int builtin_hist(char **command) {

    register HIST_ENTRY **the_list;
    register int i;
    printf("--- History --- \n");

    the_list = history_list();
    if (the_list)
        for (i = 0; the_list[i]; i++)
            printf("%d: %s\n", i + history_base, the_list[i]->line);
    else {
        printf("history could not be found!\n");
    }

    printf("--------------- \n");
    return 0;
}

#endif /*NOLIBREADLINE*/

void unquote(char *s) {
    if (s != NULL) {
        if (s[0] == '"' && s[strlen(s) - 1] == '"') {
            char *buffer = calloc(sizeof(char), strlen(s) + 1);
            strcpy(buffer, s);
            strncpy(s, buffer + 1, strlen(buffer) - 2);
            s[strlen(s) - 2] = '\0';
            free(buffer);
        }
    }
}

void unquote_command_tokens(char **tokens) {
    int i = 0;
    while (tokens[i] != NULL) {
        unquote(tokens[i]);
        i++;
    }
}

void unquote_redirect_filenames(List *redirections) {
    List *lst = redirections;
    while (lst != NULL) {
        Redirection *redirection = (Redirection *) lst->head;
        if (redirection->r_type == R_FILE) {
            unquote(redirection->u.r_file);
        }
        lst = lst->tail;
    }
}

void unquote_command(Command *cmd) {
    List *lst = NULL;
    switch (cmd->command_type) {
        case C_SIMPLE:
        case C_OR:
        case C_AND:
        case C_PIPE:
        case C_SEQUENCE:
            lst = cmd->command_sequence->command_list;
            while (lst != NULL) {
                SimpleCommand *cmd_s = (SimpleCommand *) lst->head;
                unquote_command_tokens(cmd_s->command_tokens);
                unquote_redirect_filenames(cmd_s->redirections);
                lst = lst->tail;
            }
            break;
        case C_EMPTY:
        default:
            break;
    }
}

static int execute_fork(SimpleCommand *cmd_s, int background) {
    char **command = cmd_s->command_tokens;
    pid_t pid, wpid;
    pid = fork();
    if (pid == 0) {
        /* child */
        signal(SIGINT, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);

        // Die Shell schreibt die PID und PGID des neuen Prozesses auf die
        // Statusliste.
        add_status(getpid(), getpgid(0), "running", command[0]);

        // Die Shell schreibt die PID und PGID des neuen Prozesses auf die
        // Standardfehlerausgabe.
        if (background == 1) {
            fprintf(stderr, "PID: %d PGID: %d\n", getpid(), getpgid(0));
        }

        /*
         * handle redirections here
         */
        List *list = cmd_s->redirections;
        while (list != NULL) {
            Redirection *r = ((Redirection *) list->head);
            int fd;
            switch (r->r_mode) {
                case M_READ:
                    if ((fd = open(r->u.r_file, O_RDONLY, 0777)) == -1) {
                        perror("");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDIN_FILENO);
                    break;
                case M_WRITE:
                    if ((fd = open(r->u.r_file, O_CREAT | O_WRONLY, 0777)) == -1) {
                        perror("");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDOUT_FILENO);
                    break;
                case M_APPEND:
                    if ((fd = open(r->u.r_file, O_CREAT | O_WRONLY | O_APPEND, 0777)) == -1) {
                        perror("");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDOUT_FILENO);
                    break;
            }
            close(fd);
            list = list->tail;
        }


        /*
         * remove output if background process
         */
        if (background == 1) {
            int devnull;
            if ((devnull = open("/dev/null", O_WRONLY)) == -1) {
                fprintf(stderr, "-bshell: cannot open /dev/null");
            }
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }

        if (execvp(command[0], command) == -1) {
            fprintf(stderr, "-bshell: %s : command not found \n", command[0]);
            perror("");
        }
        /*exec only return on error*/
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("shell");

    } else {
        /*parent*/
        setpgid(pid, pid);
        if (background == 0) {
            /* wait only if no background process */
            tcsetpgrp(fdtty, pid);

            /**
             * the handling here is far more complicated than this!
             * vvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
             */
            int satus = 0;
            wpid = waitpid(pid, &satus, 0);

            //^^^^^^^^^^^^^^^^^^^^^^^^^^^^

            tcsetpgrp(fdtty, shell_pid);
            return satus;
        }
    }

    return 0;
}


static int do_execute_simple(SimpleCommand *cmd_s, int background) {
    if (cmd_s == NULL) {
        return 0;
    }

    /*
     * exit codes
     */
    if (strcmp(cmd_s->command_tokens[0], "exit") == 0) {
        if (cmd_s->command_token_counter == 1)
            exit(0);
        else if (cmd_s->command_token_counter == 2)
            exit(strtol(cmd_s->command_tokens[1], NULL, 10));
        else
            fprintf(stderr, "usage: exit [CODE]\n");
        return 0;
        /*
         * change directory
         */
    } else if (strcmp(cmd_s->command_tokens[0], "cd") == 0) {
        if (cmd_s->command_token_counter == 1) {
            chdir(getenv("HOME"));
        } else if (cmd_s->command_token_counter == 2) {
            if (chdir(cmd_s->command_tokens[1]) == -1)
                fprintf(stderr, "%s: No such file or directory\nc", cmd_s->command_tokens[1]);
        } else
            fprintf(stderr, "usage: cd [DIRECTORY]\n");
        return 0;
        /*
         * print status list
         */
    } else if (strcmp(cmd_s->command_tokens[0], "status") == 0) {
        if (cmd_s->command_token_counter == 1) {
            print_status_list();
            return 0;
        } else {
            fprintf(stderr, "usage: status\n");
        }
/* do not modify this */
#ifndef NOLIBREADLINE
    } else if (strcmp(cmd_s->command_tokens[0], "hist") == 0) {
        return builtin_hist(cmd_s->command_tokens);
#endif /* NOLIBREADLINE */
    } else {
        return execute_fork(cmd_s, background);
    }
    fprintf(stderr, "This should never happen!\n");
    exit(1);
}

/*
 * check if the command is to be executed in back- or foreground.
 *
 * For sequences, the '&' sign of the last command in the
 * sequence is checked.
 *
 * returns:
 *      0 in case of foreground execution
 *      1 in case of background execution
 *
 */
int check_background_execution(Command *cmd) {
    List *lst = NULL;
    int background = 0;
    switch (cmd->command_type) {
        case C_SIMPLE:
            lst = cmd->command_sequence->command_list;
            background = ((SimpleCommand *) lst->head)->background;
            break;
        case C_OR:
        case C_AND:
        case C_PIPE:
        case C_SEQUENCE:
            /*
             * last command in sequence defines whether background or
             * forground execution is specified.
             */
            lst = cmd->command_sequence->command_list;
            while (lst != NULL) {
                background = ((SimpleCommand *) lst->head)->background;
                lst = lst->tail;
            }
            break;
        case C_EMPTY:
        default:
            break;
    }
    return background;
}


int execute(Command *cmd) {
    unquote_command(cmd);

    int res = 0;
    List *lst = NULL;

    int command_list_len = cmd->command_sequence->command_list_len;
    int fd_list_len = command_list_len - 1;
    int fd[fd_list_len][2];
    int pid[command_list_len];

    int execute_in_background = check_background_execution(cmd);
    switch (cmd->command_type) {
        case C_EMPTY:
            break;
        case C_SIMPLE:
            res = do_execute_simple((SimpleCommand *) cmd->command_sequence->command_list->head,
                                    execute_in_background);
            fflush(stderr);
            break;

        case C_OR:
            lst = cmd->command_sequence->command_list;
            while (lst != NULL) {
                if ((res = do_execute_simple((SimpleCommand *) lst->head, execute_in_background)) == 0) {
                    return 0;
                }
                fflush(stderr);
                lst = lst->tail;
            }
        case C_AND:
            lst = cmd->command_sequence->command_list;
            while (lst != NULL) {
                if ((res = do_execute_simple((SimpleCommand *) lst->head, execute_in_background)) != 0) {
                    return 0;
                }
                fflush(stderr);
                lst = lst->tail;
            }
        case C_SEQUENCE:
            lst = cmd->command_sequence->command_list;
            while (lst != NULL) {
                res = do_execute_simple((SimpleCommand *) lst->head, execute_in_background);
                fflush(stderr);
                lst = lst->tail;
            }
            break;
        case C_PIPE:
            lst = cmd->command_sequence->command_list;

            /* create a pipe for every command -1 and check valid */
            for (int i = 0; i < fd_list_len; i++) {
                if ((pipe(fd[i])) == -1) {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }
            }

            /*
             * first child process
             * 1. close unused pipes
             * 2. dup write_next fd to STDOUT_FILENO
             * 3. exec command and error
             * 4. close write_next fd
             */
            if ((pid[0] = fork()) == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid[0] == 0) {
                signal(SIGINT, SIG_DFL);
                for (int j = 0; j < fd_list_len; j++) {
                    if (j == 0) {
                        close(fd[j][0]);
                    } else {
                        close(fd[j][0]);
                        close(fd[j][1]);
                    }
                }
                dup2(fd[0][1], STDOUT_FILENO);
                if (execvp(((SimpleCommand *) lst->head)->command_tokens[0],
                           ((SimpleCommand *) lst->head)->command_tokens) == -1) {
                    fprintf(stderr, "-bshell: %s : command not found \n",
                            ((SimpleCommand *) lst->head)->command_tokens[0]);
                    perror("");
                }
                close(fd[0][1]);
                //exit(0);
            }
            lst = lst->tail;

            /*
             * middle child process
             * 1. close unused pipes
             * 2. dup read_before fd to STDIN_FILENO
             * 3. dup write_next fd to STDOUT_FILENO
             * 4. exec command and error
             * 5. close read_before & write_next
             */
            for (int i = 1; i < fd_list_len; i++) {
                if ((pid[i] = fork()) == -1) {
                    perror("fork");
                    exit(EXIT_FAILURE);
                } else if (pid[i] == 0) {
                    signal(SIGINT, SIG_DFL);
                    for (int j = 0; j < fd_list_len; j++) {
                        if (j == i - 1) {
                            close(fd[j][1]);
                        } else if (j == i) {
                            close(fd[j][0]);
                        } else {
                            close(fd[j][0]);
                            close(fd[j][1]);
                        }
                    }
                    dup2(fd[i - 1][0], STDIN_FILENO);
                    dup2(fd[i][1], STDOUT_FILENO);
                    if (execvp(((SimpleCommand *) lst->head)->command_tokens[0],
                               ((SimpleCommand *) lst->head)->command_tokens) == -1) {
                        fprintf(stderr, "-bshell: %s : command not found \n",
                                ((SimpleCommand *) lst->head)->command_tokens[0]);
                        perror("");
                    }
                    close(fd[i - 1][0]);
                    close(fd[i][1]);
                    //exit(0);
                }
                lst = lst->tail;
            }


            /*
             * last child process
             * 1. close unused pipes
             * 2. dup read_before fd to STDIN_FILENO
             * 3. exec command and error
             * 4. close read_before fd
             */
            if ((pid[command_list_len - 1] = fork()) == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid[command_list_len - 1] == 0) {
                signal(SIGINT, SIG_DFL);
                for (int j = 0; j < fd_list_len; j++) {
                    if (j == fd_list_len - 1) {
                        close(fd[j][1]);
                    } else {
                        close(fd[j][0]);
                        close(fd[j][1]);
                    }
                }
                dup2(fd[fd_list_len - 1][0], STDIN_FILENO);
                if (execvp(((SimpleCommand *) lst->head)->command_tokens[0],
                           ((SimpleCommand *) lst->head)->command_tokens) == -1) {
                    fprintf(stderr, "-bshell: %s : command not found \n",
                            ((SimpleCommand *) lst->head)->command_tokens[0]);
                    perror("");
                }
                close(fd[fd_list_len - 1][0]);
                //exit(0);
            }

            for (int j = 0; j < fd_list_len; j++) {
                close(fd[j][0]);
                close(fd[j][1]);
            }

            for (int i = 0; i < command_list_len; i++) {
                waitpid(pid[i], NULL, 0);
            }
            break;
        default:
            printf("[%s] unhandled command type [%i]\n", __func__, cmd->command_type);
            break;
    }
    return res;
}

