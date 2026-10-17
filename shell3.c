#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define IA 128
#define PATH_BUF 1024
#define CMD_BUF 512
#define MAX_PIPES 16

extern char **environ;

/* manual replacement for getenv("HOME") - scans environ directly */
char *get_home(void) {
    for (int i = 0; environ[i] != NULL; i++) {
        if (strncmp(environ[i], "HOME=", 5) == 0) {
            return environ[i] + 5;
        }
    }
    return NULL;
}

/* searches PATH (colon separated, in path_storage) for an executable
   named cmdname. On success writes the full path into outbuf and
   returns 1. On failure returns 0. */
int find_command(char *cmdname, char *PATH, char *outbuf) {
    char path_copy[PATH_BUF];
    strncpy(path_copy, PATH, PATH_BUF - 1);
    path_copy[PATH_BUF - 1] = '\0';

    char *cpath[IA];
    int npaths = 0;
    char *pathToken = strtok(path_copy, ":\n");

    while (pathToken != NULL && npaths < IA - 1) {
        cpath[npaths] = pathToken;
        npaths++;
        pathToken = strtok(NULL, ":\n");
    }
    cpath[npaths] = NULL;

    for (int i = 0; i < npaths; i++) {
        if (strlen(cpath[i]) + strlen(cmdname) + 2 >= CMD_BUF)
            continue;

        strcpy(outbuf, cpath[i]);
        strcat(outbuf, "/");
        strcat(outbuf, cmdname);

        if (access(outbuf, X_OK) == 0) {
            return 1;
        }
        outbuf[0] = '\0';
    }

    return 0;
}

/* runs the resolved executable at cmd with the given args/argc,
   using execl exactly as shell2.c did. Only returns on failure
   (execl does not return on success). */
void run_command(char *cmd, char **args, int argc) {
    switch (argc) {
        case 1:
            execl(cmd, args[0], (char *) NULL);
            break;
        case 2:
            execl(cmd, args[0], args[1], (char *) NULL);
            break;
        case 3:
            execl(cmd, args[0], args[1], args[2], (char *) NULL);
            break;
        case 4:
            execl(cmd, args[0], args[1], args[2], args[3], (char *) NULL);
            break;
        case 5:
            execl(cmd, args[0], args[1], args[2], args[3], args[4], (char *) NULL);
            break;
        case 6:
            execl(cmd, args[0], args[1], args[2], args[3], args[4], args[5], (char *) NULL);
            break;
        case 7:
            execl(cmd, args[0], args[1], args[2], args[3], args[4], args[5], args[6], (char *) NULL);
            break;
        case 8:
            execl(cmd, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], (char *) NULL);
            break;
        case 9:
            execl(cmd, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], (char *) NULL);
            break;
        default: {
            char *msg = "Too many arguments for exec\n";
            write(2, msg, strlen(msg));
            _exit(1);
        }
    }

    char *msg = "Command not found\n";
    write(2, msg, strlen(msg));
    _exit(1);
}

int main() {
    char input[IA];
    char *args[IA];
    char *prompt = "customshell$ ";

    char path_storage[PATH_BUF] =
        "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/snap/bin";
    char *PATH = path_storage;

    while (1) {
        write(1, prompt, strlen(prompt));

        int bytes = read(0, input, IA - 1);

        if (bytes == 0) {
            write(1, "\n", 1);
            break;
        }

        if (bytes < 0) {
            char *msg = "Error reading input\n";
            write(2, msg, strlen(msg));
            continue;
        }

        input[bytes] = '\0';

        if (bytes > 0 && input[bytes - 1] == '\n') {
            input[bytes - 1] = '\0';
        } else if (bytes == IA - 1) {
            char *msg = "Warning: input line too long, truncated\n";
            write(2, msg, strlen(msg));

            char drain[IA];
            int d;
            do {
                d = read(0, drain, IA - 1);
            } while (d == IA - 1);
        }

        int argc = 0;
        char *token = strtok(input, " =\t\n");

        while (token != NULL && argc < IA - 1) {
            args[argc] = token;
            argc++;
            token = strtok(NULL, " =\t\n");
        }

        args[argc] = NULL;

        if (argc == 0)
            continue;

        if (strcmp(args[0], "exit") == 0)
            break;

        if (strcmp(args[0], "cd") == 0) {
            if (argc == 1) {
                char *home = get_home();

                if (home != NULL) {
                    if (chdir(home) < 0) {
                        char *msg = "cd failed\n";
                        write(2, msg, strlen(msg));
                    }
                }
            }
            else if (argc == 2) {
                if (chdir(args[1]) < 0) {
                    char *msg = "Directory not found\n";
                    write(2, msg, strlen(msg));
                }
            }
            else {
                char *msg = "Too many arguments\n";
                write(2, msg, strlen(msg));
            }

            continue;
        }

        if (strcmp(args[0], "PATH") == 0) {

            char *newpath = NULL;

            if (argc >= 2) {
                newpath = args[1];
            }

            if (newpath == NULL || newpath[0] == '\0') {
                char *msg = "Wrong\n";
                write(1, msg, strlen(msg));
            } else {
                strncpy(path_storage, newpath, PATH_BUF - 1);
                path_storage[PATH_BUF - 1] = '\0';
                PATH = path_storage;
            }

            continue;
        }

        /* split args into a pipeline of commands, wherever "|" appears */
        char **cmdv[MAX_PIPES];
        int cmdc[MAX_PIPES];
        int ncmds = 0;

        cmdv[0] = &args[0];
        cmdc[0] = 0;

        int bad_pipe = 0;

        for (int i = 0; i < argc; i++) {
            if (strcmp(args[i], "|") == 0) {
                args[i] = NULL;

                if (cmdc[ncmds] == 0) {
                    bad_pipe = 1;
                    break;
                }

                ncmds++;

                if (ncmds >= MAX_PIPES) {
                    bad_pipe = 1;
                    break;
                }

                cmdv[ncmds] = &args[i + 1];
                cmdc[ncmds] = 0;
            } else {
                cmdc[ncmds]++;
            }
        }

        if (!bad_pipe && cmdc[ncmds] == 0) {
            bad_pipe = 1;
        }

        if (bad_pipe) {
            char *msg = "Invalid pipe syntax\n";
            write(2, msg, strlen(msg));
            continue;
        }

        ncmds++;

        /* single command, no pipe: behave exactly like shell2.c */
        if (ncmds == 1) {
            char cmd[CMD_BUF] = "";

            if (!find_command(args[0], PATH, cmd)) {
                char *msg = "Command not found\n";
                write(2, msg, strlen(msg));
                continue;
            }

            int pid = fork();

            if (pid < 0) {
                char *msg = "Fork failed\n";
                write(2, msg, strlen(msg));
                continue;
            }

            if (pid == 0) {
                run_command(cmd, args, argc);
            }
            else {
                wait(NULL);
            }

            continue;
        }

        /* multiple commands pipe */
        char resolved[MAX_PIPES][CMD_BUF];
        int resolve_failed = 0;

        for (int i = 0; i < ncmds; i++) {
            resolved[i][0] = '\0';
            if (!find_command(cmdv[i][0], PATH, resolved[i])) {
                char *msg = "Command not found\n";
                write(2, msg, strlen(msg));
                resolve_failed = 1;
                break;
            }
        }

        if (resolve_failed)
            continue;

        int pipefds[MAX_PIPES - 1][2];

        for (int i = 0; i < ncmds - 1; i++) {
            if (pipe(pipefds[i]) < 0) {
                char *msg = "Pipe creation failed\n";
                write(2, msg, strlen(msg));

                for (int j = 0; j < i; j++) {
                    close(pipefds[j][0]);
                    close(pipefds[j][1]);
                }

                resolve_failed = 1;
                break;
            }
        }

        if (resolve_failed)
            continue;

        pid_t pids[MAX_PIPES];

        for (int i = 0; i < ncmds; i++) {
            pid_t pid = fork();

            if (pid < 0) {
                char *msg = "Fork failed\n";
                write(2, msg, strlen(msg));
                continue;
            }

            if (pid == 0) {
                if (i > 0) {
                    dup2(pipefds[i - 1][0], 0);
                }
                if (i < ncmds - 1) {
                    dup2(pipefds[i][1], 1);
                }

                for (int j = 0; j < ncmds - 1; j++) {
                    close(pipefds[j][0]);
                    close(pipefds[j][1]);
                }

                int this_argc = 0;
                while (cmdv[i][this_argc] != NULL)
                    this_argc++;

                run_command(resolved[i], cmdv[i], this_argc);
            }
            else {
                pids[i] = pid;
            }
        }

        for (int i = 0; i < ncmds - 1; i++) {
            close(pipefds[i][0]);
            close(pipefds[i][1]);
        }

        for (int i = 0; i < ncmds; i++) {
            wait(NULL);
        }
    }

    return 0;
}
