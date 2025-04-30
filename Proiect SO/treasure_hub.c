// treasure_hub.c
// Phase 2 interactive hub with background monitor using signals

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#define CMD_FILE  "hub_cmd.txt"

static pid_t monitor_pid = -1;
static int   monitor_running = 0;

// SIGCHLD handler: catch any child’s exit
void sigchld_handler(int sig) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (pid == monitor_pid) {
            monitor_running = 0;
            if (WIFEXITED(status)) {
                printf("\nMonitor exited with status %d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("\nMonitor killed by signal %d\n", WTERMSIG(status));
            }
        }
    }
}

void start_monitor() {
    if (monitor_running) {
        printf("Monitor already running (PID %d)\n", monitor_pid);
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        // Child: redirect stdout/stderr and exec the monitor
        fclose(stdout);
        fclose(stderr);
        open("/dev/null", O_WRONLY); // fd 1 = stdout
        dup2(1, 2); // stderr -> stdout

        execl("./treasure_monitor", "treasure_monitor", NULL);
        perror("execl");
        exit(1);
    }

    // Parent continues
    monitor_pid = pid;
    monitor_running = 1;

    struct sigaction sa = {
        .sa_handler = sigchld_handler,
        .sa_flags   = SA_NOCLDSTOP
    };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    printf("Started monitor (PID %d)\n", monitor_pid);
}

void send_cmd(const char *cmd) {
    if (!monitor_running) {
        printf("Error: monitor not running.\n");
        return;
    }
    FILE *f = fopen(CMD_FILE, "w");
    if (!f) { perror("fopen"); return; }

    fprintf(f, "%s\n", cmd);
    fflush(f);
    int fd = fileno(f);
    fsync(fd);
    fclose(f);

    kill(monitor_pid, SIGUSR1);
}

void stop_monitor() {
    if (!monitor_running) {
        printf("Monitor is not running.\n");
        return;
    }
    kill(monitor_pid, SIGUSR2);
    printf("Sent stop request to monitor.\n");
}

int main() {
    char line[128];

    while (1) {
        printf("\nhub> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = '\0';

        if      (strcmp(line, "start_monitor") == 0) start_monitor();
        else if (strcmp(line, "list_hunts")    == 0) send_cmd("list_hunts");
        else if (strncmp(line, "list_treasures ", 15) == 0) send_cmd(line);
        else if (strncmp(line, "view_treasure ", 14) == 0) send_cmd(line);
        else if (strcmp(line, "stop_monitor")   == 0) stop_monitor();
        else if (strcmp(line, "status")         == 0) {
            if (monitor_running)
                printf("Monitor is running (PID %d)\n", monitor_pid);
            else
                printf("Monitor is not running.\n");
        }
        else if (strcmp(line, "exit")           == 0) {
            if (monitor_running)
                printf("Error: stop monitor before exit.\n");
            else
                break;
        }
        else {
            printf("Unknown command: %s\n", line);
        }
    }
    return 0;
}
