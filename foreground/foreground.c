#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <libgen.h>

struct state {
    char command[2048];
    char pid_file[2048];
    pid_t pid;
} state;

int parse_commandline(int argc, char **argv)
{
    for (int ch = 0; ch != -1; ch = getopt(argc, argv, "c:p:")) {
        switch (ch) {
            case 'c':
                (void) strcpy(state.command, optarg);
                break;

            case 'p':
                (void) strcpy(state.pid_file, optarg);
                break;

            case ':':
            case '?':
                goto error;

            default:
                break;
        }
    }

    if (strlen(state.command) == 0 || strlen(state.pid_file) == 0) {
        (void) fprintf(stderr, "The options -c and -p need to be specified.\n");
        goto error;
    }

    return 0;

error:
    return -1;
}

void signal_handler(int signum)
{
    if (SIGCHLD == signum) {
        return;
    }

    if (kill(state.pid, signum) == -1) {
        perror("kill");
    }

    if (SIGINT == signum || SIGTERM == signum) {
        exit(EXIT_SUCCESS);
    }
}

int register_signals()
{
    struct sigaction sa;
    sa.sa_handler = signal_handler;

    for (int i = 1; i <= 31; i++) {
        if (SIGKILL == i || SIGSTOP == i || SIGCHLD == i) {
            continue;
        }

        if (sigaction(i, &sa, NULL) == -1) {
            perror("sigaction");
            return -1;
        }
    }

    return 0;
}

pid_t start_process(const char *command)
{
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return -1;
    }

    if (pid != 0) {
        return pid;
    }

    exit(system(command));
}

int read_pid_file(const char *pid_file)
{
    FILE *fp = fopen(pid_file, "r");
    if (!fp) {
        perror("fopen");
        return -1;
    }

    int buffer_size = 6;
    char buffer[buffer_size];
    if (!fgets(buffer, buffer_size, fp)) {
        (void) fclose(fp);
        return -2;
    }

    (void) fclose(fp);

    return atoi(buffer);
}

int main(int argc, char **argv)
{
    state.command[0] = '\0';
    state.pid_file[0] = '\0';

    if (parse_commandline(argc, argv) == -1) {
        goto error;
    }

    if (start_process(state.command) == -1) {
        goto error;
    }

    if (register_signals() == -1) {
        goto error;
    }

    sleep(2);

    state.pid = read_pid_file(state.pid_file);
    if (state.pid <= 0) {
        goto error;
    }

    while (access(state.pid_file, F_OK) == 0 &&
            kill(state.pid, 0) == 0) {
        sleep(1);
    }

    return EXIT_SUCCESS;

error:
    return EXIT_FAILURE;
}
