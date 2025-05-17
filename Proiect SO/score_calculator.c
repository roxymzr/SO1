#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct {
    char id[256];
    char username[256];
    float latitude;
    float longitude;
    char clue[256];
    int value;
} Treasure;

typedef struct {
    char username[256];
    int score;
} Score;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/treasure.dat", argv[1]);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    Score scores[100];
    int count = 0;

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        int found = 0;
        for (int i = 0; i < count; ++i) {
            if (strcmp(scores[i].username, t.username) == 0) {
                scores[i].score += t.value;
                found = 1;
                break;
            }
        }
        if (!found) {
            strncpy(scores[count].username, t.username, 255);
            scores[count].score = t.value;
            count++;
        }
    }
    close(fd);

    for (int i = 0; i < count; ++i)
        printf("%s: %d\n", scores[i].username, scores[i].score);

    return 0;
}
