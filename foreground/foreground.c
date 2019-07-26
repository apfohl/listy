#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <libgen.h>

struct configuration {
    char *command;
    char *pid_file;
};

struct state {
    struct configuration *configuration;
    pid_t pid;
} state;

void cleanup_state()
{
    if (state.configuration) {
        if (state.configuration->command) {
            free(state.configuration->command);
        }

        if (state.configuration->pid_file) {
            free(state.configuration->pid_file);
        }

        free(state.configuration);
        state.configuration = NULL;
    }
}

struct configuration *parse_commandline(int argc, char **argv)
{
    struct configuration *configuration =
        calloc(1, sizeof(struct configuration));
    if (!configuration) {
        goto error;
    }

    for (int ch = 0; ch != -1; ch = getopt(argc, argv, "c:p:")) {
        switch (ch) {
            case 'c':
                configuration->command = strdup(optarg);
                break;

            case 'p':
                configuration->pid_file = strdup(optarg);
                break;

            case ':':
            case '?':
                goto error;

            default:
                break;
        }
    }

    if (configuration->command == NULL || configuration->pid_file == NULL) {
        (void) fprintf(stderr, "The options -c and -p need to be specified.\n");
        goto error;
    }

    goto success;

error:
    if (configuration->command) {
        free(configuration->command);
    }

    if (configuration->pid_file) {
        free(configuration->pid_file);
    }

    free(configuration);
    configuration = NULL;

success:
    return configuration;
}

void signal_handler(int signum)
{
    if (kill(state.pid, signum) == -1) {
        perror("kill");
    }

    if (SIGINT == signum || SIGTERM == signum) {
        cleanup_state();
        exit(EXIT_SUCCESS);
    }
}

int register_signals()
{
    struct sigaction sa;
    sa.sa_handler = signal_handler;

    for (int i = 1; i <= 31; i++) {
        if (SIGKILL == i || SIGSTOP == i) {
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

    char command_copy[1024];

    strcpy(command_copy, command);
    char *dir = dirname(command_copy);
    strcpy(command_copy, command);
    char *base = basename(command_copy);

    cleanup_state();

    if (execl(dir, base, NULL) == -1) {
        perror("execl");
        exit(EXIT_FAILURE);
    }

    return 0;
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
    state.configuration = parse_commandline(argc, argv);
    if (!state.configuration) {
        goto error;
    }

    if (register_signals() == -1) {
        goto error;
    }

    if (start_process(state.configuration->command) == -1) {
        goto error;
    }

    state.pid = read_pid_file(state.configuration->pid_file);
    if (state.pid < 0) {
        goto error;
    }

    while (1) {
        sleep(1);
    }

    goto success;

error:
    cleanup_state();
    return EXIT_FAILURE;

success:
    return EXIT_SUCCESS;
}
