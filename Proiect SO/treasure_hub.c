#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>

#define CMD_FILE  "hub_cmd.txt"

static pid_t monitor_pid = -1;
static int   monitor_running = 0;
int pipefd[2];

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
        printf("Monitor already running.\n");
        return;
    }
    if (pipe(pipefd) < 0) {
        perror("pipe");
        return;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[0]);
        execl("./treasure_monitor", "treasure_monitor", NULL);
        perror("exec");
        exit(1);
    }
    close(pipefd[1]);
    monitor_pid = pid;
    monitor_running = 1;

    struct sigaction sa = { .sa_handler = sigchld_handler };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    printf("Monitor started (PID %d)\n", monitor_pid);
}

void send_cmd(const char *cmd) {
    if (!monitor_running) {
        printf("Monitor is not running.\n");
        return;
    }
    FILE *f = fopen(CMD_FILE, "w");
    if (!f) { perror("fopen"); return; }
    fprintf(f, "%s\n", cmd);
    fclose(f);
    kill(monitor_pid, SIGUSR1);
    usleep(200000);
    char buf[512];
    ssize_t n;
    while ((n = read(pipefd[0], buf, sizeof(buf)-1)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
        if (n < sizeof(buf)-1) break;
    }
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

        if (strcmp(line, "start_monitor") == 0) start_monitor();
        else if (strcmp(line, "list_hunts") == 0) send_cmd("list_hunts");
        else if (strncmp(line, "list_treasures ", 15) == 0) send_cmd(line);
        else if (strncmp(line, "view_treasure ", 14) == 0) send_cmd(line);
        else if (strcmp(line, "stop_monitor") == 0) stop_monitor();
        else if (strcmp(line, "status") == 0) {
            if (monitor_running)
                printf("Monitor is running (PID %d)\n", monitor_pid);
            else
                printf("Monitor is not running.\n");
        } else if (strcmp(line, "calculate_score") == 0) {
            DIR *dir = opendir(".");
            struct dirent *entry;
            while ((entry = readdir(dir))) {
                if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
                    int pfd[2];
                    pipe(pfd);
                    pid_t pid = fork();
                    if (pid == 0) {
                        dup2(pfd[1], STDOUT_FILENO);
                        close(pfd[0]);
                        execl("./score_calculator", "score_calculator", entry->d_name, NULL);
                        exit(1);
                    }
                    close(pfd[1]);
                    char buf[256];
                    printf("Scores for %s:\n", entry->d_name);
                    ssize_t n;
                    while ((n = read(pfd[0], buf, sizeof(buf)-1)) > 0) {
                        buf[n] = '\0';
                        printf("%s", buf);
                    }
                    close(pfd[0]);
                    waitpid(pid, NULL, 0);
                }
            }
            closedir(dir);
        } else if (strcmp(line, "exit") == 0) {
            if (monitor_running)
                printf("Error: stop monitor before exit.\n");
            else
                break;
        } else {
            printf("Unknown command: %s\n", line);
        }
    }
    return 0;
}