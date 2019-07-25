#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

struct configuration
{
    char *command;
    char *pid_file;
};

struct configuration *parse_commandline(int argc, char **argv) {
    struct configuration *configuration =
        calloc(1, sizeof(struct configuration));

    for (int ch = 0; ch != -1; ch = getopt(argc, argv, "c:p:")) {
        switch (ch)
        {
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

void signal_handler(int signum) {
	if (SIGKILL == signum || SIGTERM == signum) {
		exit(0);
	}
}

int register_signals() {
    struct sigaction sa;
	sa.sa_handler = signal_handler;

	for (int i = 1; i <= 31; i++) {
        if (SIGKILL == i || SIGSTOP == i) continue;

		if (sigaction(i, &sa, NULL) == -1) {
            perror("sigaction");
            return -1;
        }
	}

    return 0;
}

int main(int argc, char **argv) {
    struct configuration *configuration = parse_commandline(argc, argv);
    if (!configuration) {
        return EXIT_FAILURE;
    }

	if (register_signals() == -1) {
        free(configuration);
        return EXIT_FAILURE;
    }

	while(1) {
		sleep(1);
	}

	return EXIT_SUCCESS;
}
