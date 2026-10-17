#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define IA 128

int main() {
    char input[IA];
    char *args[IA];
    char *prompt = "customshell$ ";

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

        if (input[bytes - 1] == '\n') {
            input[bytes - 1] = '\0';
        }

        int argc = 0;
        char *token = strtok(input, " \t\n");

        while (token != NULL && argc < IA - 1) {
            args[argc] = token;
            argc++;
            token = strtok(NULL, " \t\n");
        }

        args[argc] = NULL;

        if (argc == 0)
            continue;

        if (strcmp(args[0], "exit") == 0)
            break;

        if (strcmp(args[0], "cd") == 0) {
            if (argc == 1) {
                char *home = getenv("HOME");

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

        int pid = fork();

        if (pid < 0) {
            char *msg = "Fork failed\n";
            write(2, msg, strlen(msg));
            continue;
        }

        if (pid == 0) {
            execvp(args[0], args);

            char *msg = "Command not found\n";
            write(2, msg, strlen(msg));

            _exit(1);
        }
        else {
            wait(NULL);
        }
    }

    return 0;
}
