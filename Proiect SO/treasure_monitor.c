// === treasure_monitor.c ===
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>

#define CMD_FILE      "hub_cmd.txt"
#define TREASURE_EXE  "./treasure_manager"
#define PATH_SIZE     512
#define BUFFER_SIZE   512
#define ID_SIZE       256

typedef struct {
    char id[ID_SIZE];
    char username[ID_SIZE];
    float latitude;
    float longitude;
    char clue[ID_SIZE];
    int value;
} Treasure;

static volatile sig_atomic_t got_cmd_signal   = 0;
static volatile sig_atomic_t got_stop_signal  = 0;

void handle_cmd(int sig) { got_cmd_signal = 1; }
void handle_stop(int sig) { got_stop_signal = 1; }

void write_output(const char *msg) {
    write(STDOUT_FILENO, msg, strlen(msg));
}

void exec_command(char *const argv[]) {
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe");
        return;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    } else if (pid == 0) {
        // child: run treasure_manager command
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        execvp(argv[0], argv);
        perror("execvp");
        exit(1);
    } else {
        // parent: read child's output and forward
        close(pipefd[1]);
        char buffer[BUFFER_SIZE];
        ssize_t n;
        while ((n = read(pipefd[0], buffer, BUFFER_SIZE)) > 0) {
            write(STDOUT_FILENO, buffer, n);
        }
        close(pipefd[0]);
        waitpid(pid, NULL, 0);
    }
}

void list_hunts_pipe() {
    DIR *dir = opendir(".");
    if (!dir) return;

    struct dirent *entry;
    char path[PATH_SIZE];
    char buffer[BUFFER_SIZE];

    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
            snprintf(path, sizeof(path), "%s/treasure.dat", entry->d_name);
            int fd = open(path, O_RDONLY);
            if (fd < 0) continue;

            int count = 0;
            Treasure t;
            while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure))
                count++;
            close(fd);

            snprintf(buffer, sizeof(buffer), "%s: %d treasures\n", entry->d_name, count);
            write_output(buffer);
        }
    }

    closedir(dir);
}

void process_command() {
    FILE *f = fopen(CMD_FILE, "r");
    if (!f) return;

    char buf[BUFFER_SIZE];
    if (!fgets(buf, BUFFER_SIZE, f)) {
        fclose(f);
        return;
    }
    fclose(f);
    buf[strcspn(buf, "\n")] = '\0';

    char *cmd = strtok(buf, " ");
    if (!cmd) return;

    if (strcmp(cmd, "list_hunts") == 0) {
        list_hunts_pipe();
    } else if (strcmp(cmd, "list_treasures") == 0) {
        char *hunt = strtok(NULL, " ");
        if (hunt) {
            char *args[] = {TREASURE_EXE, "--list", hunt, NULL};
            exec_command(args);
        }
    } else if (strcmp(cmd, "view_treasure") == 0) {
        char *hunt = strtok(NULL, " ");
        char *tid = strtok(NULL, " ");
        if (hunt && tid) {
            char *args[] = {TREASURE_EXE, "--view", hunt, tid, NULL};
            exec_command(args);
        }
    } else {
        write_output("Monitor: Unknown command\n");
    }
}

int main() {
    // Setup signal handlers
    struct sigaction sa1;
    memset(&sa1, 0, sizeof(sa1));
    sa1.sa_handler = handle_cmd;
    sigaction(SIGUSR1, &sa1, NULL);

    struct sigaction sa2;
    memset(&sa2, 0, sizeof(sa2));
    sa2.sa_handler = handle_stop;
    sigaction(SIGUSR2, &sa2, NULL);

    // Main loop
    while (1) {
        pause();
        if (got_cmd_signal) {
            process_command();
            got_cmd_signal = 0;
        }
        if (got_stop_signal) {
            usleep(500000); // simulate cleanup
            write_output("Monitor terminating...\n");
            exit(0);
        }
    }
    return 0;
}
