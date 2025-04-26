// treasure_hub.c
// Phase 2 interactive hub with background monitor using signals

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#define CMD_FILE  "hub_cmd.txt"

static pid_t monitor_pid = -1;
static int   monitor_running = 0;

// SIGCHLD handler: catch the monitor’s exit
void sigchld_handler(int sig) {
    int status;
    pid_t pid = waitpid(monitor_pid, &status, WNOHANG);
    if (pid == monitor_pid) {
        monitor_running = 0;
        if (WIFEXITED(status)) {
            printf("Monitor exited with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Monitor killed by signal %d\n", WTERMSIG(status));
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
        // Child → exec the monitor
        execl("./treasure_monitor", "treasure_monitor", NULL);
        perror("execl");
        exit(1);
    }
    // Parent
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

// Write a single-line command to CMD_FILE and signal monitor
void send_cmd(const char *cmd) {
    if (!monitor_running) {
        printf("Error: monitor not running.\n");
        return;
    }
    FILE *f = fopen(CMD_FILE, "w");
    if (!f) { perror("fopen"); return; }
    fprintf(f, "%s\n", cmd);
    fclose(f);

    // Tell monitor to read it
    kill(monitor_pid, SIGUSR1);
}

void stop_monitor() {
    if (!monitor_running) {
        printf("Monitor is not running.\n");
        return;
    }
    kill(monitor_pid, SIGUSR2);
    printf("Sent stop request to monitor.\n");
    // Further commands will be blocked until it exits (SIGCHLD)
}

int main() {
    char line[128];

    while (1) {
        printf("hub> ");
        fflush(stdout);               // <— force the prompt to appear immediately
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = '\0';

        if      (strcmp(line, "start_monitor") == 0) start_monitor();
        else if (strcmp(line, "list_hunts")    == 0) send_cmd("list_hunts");
        else if (strncmp(line, "list_treasures ", 15) == 0) send_cmd(line);
        else if (strncmp(line, "view_treasure ", 14) == 0) send_cmd(line);
        else if (strcmp(line, "stop_monitor")   == 0) stop_monitor();
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
