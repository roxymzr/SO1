// treasure_monitor.c
// Background monitor process for Phase 2

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <dirent.h>

#define CMD_FILE      "hub_cmd.txt"
#define TREASURE_EXE  "./treasure_manager"

static volatile sig_atomic_t got_cmd_signal   = 0;
static volatile sig_atomic_t got_stop_signal  = 0;

void handle_cmd(int sig) {
    got_cmd_signal = 1;
}

void handle_stop(int sig) {
    got_stop_signal = 1;
}

void exec_command(char *const argv[]) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("monitor: fork");
        return;
    } else if (pid == 0) {
        execvp(argv[0], argv);
        perror("monitor: execvp");
        exit(1);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

void list_hunts() {
    printf("== Hunts & counts ==\n");

    DIR *dir = opendir(".");
    if (!dir) {
        perror("monitor: opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
            char *hunt_name = entry->d_name;
            pid_t pid = fork();
            if (pid < 0) {
                perror("monitor: fork");
                continue;
            } else if (pid == 0) {
                // child: exec treasure_manager --list <hunt> | grep -c '^>>'
                int pipefd[2];
                pipe(pipefd);
                pid_t gpid = fork();
                if (gpid == 0) {
                    // grandchild: exec treasure_manager
                    dup2(pipefd[1], STDOUT_FILENO);
                    close(pipefd[0]);
                    execl(TREASURE_EXE, TREASURE_EXE, "--list", hunt_name, NULL);
                    perror("monitor: exec treasure_manager");
                    exit(1);
                } else {
                    // child: read output and count lines with ">>"
                    close(pipefd[1]);
                    FILE *fp = fdopen(pipefd[0], "r");
                    char line[256];
                    int count = 0;
                    while (fgets(line, sizeof(line), fp)) {
                        if (strncmp(line, ">>", 2) == 0) count++;
                    }
                    fclose(fp);
                    wait(NULL); // wait for grandchild
                    printf("%s: %d treasures\n", hunt_name, count);
                    exit(0);
                }
            } else {
                waitpid(pid, NULL, 0);
            }
        }
    }
    closedir(dir);
}

void process_command() {
    FILE *f = fopen(CMD_FILE, "r");
    if (!f) {
        perror("monitor: fopen");
        return;
    }
    char buf[256];
    if (!fgets(buf, sizeof(buf), f)) {
        fclose(f);
        return;
    }
    fclose(f);
    buf[strcspn(buf, "\n")] = '\0';

    char *cmd = strtok(buf, " ");
    if (!cmd) return;

    if (strcmp(cmd, "list_hunts") == 0) {
        list_hunts();
    }
    else if (strcmp(cmd, "list_treasures") == 0) {
        char *hunt = strtok(NULL, " ");
        if (hunt) {
            printf("== Treasures in %s ==\n", hunt);
            char *args[] = {TREASURE_EXE, "--list", hunt, NULL};
            exec_command(args);
        } else {
            printf("Monitor: missing hunt name for list_treasures.\n");
        }
    }
    else if (strcmp(cmd, "view_treasure") == 0) {
        char *hunt = strtok(NULL, " ");
        char *tid  = strtok(NULL, " ");
        if (hunt && tid) {
            printf("== View %s / %s ==\n", hunt, tid);
            char *args[] = {TREASURE_EXE, "--view", hunt, tid, NULL};
            exec_command(args);
        } else {
            printf("Monitor: missing arguments for view_treasure.\n");
        }
    }
    else {
        printf("Monitor: unknown command '%s'\n", cmd);
    }
}

int main() {
    struct sigaction sa1 = { .sa_handler = handle_cmd };
    sigemptyset(&sa1.sa_mask);
    sa1.sa_flags = 0;
    sigaction(SIGUSR1, &sa1, NULL);

    struct sigaction sa2 = { .sa_handler = handle_stop };
    sigemptyset(&sa2.sa_mask);
    sa2.sa_flags = 0;
    sigaction(SIGUSR2, &sa2, NULL);

    printf("Monitor ready (pid=%d)\n", getpid());
    fflush(stdout);

    while (1) {
        pause();  // wait for signal

        if (got_cmd_signal) {
            process_command();
            got_cmd_signal = 0;
        }
        if (got_stop_signal) {
            usleep(500000);  // simulate cleanup
            printf("Monitor terminating...\n");
            fflush(stdout);
            exit(0);
        }
    }
    return 0;
}

