#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>

#define SIZE 256
#define HUNT_FILE "hunt.log"
#define TREASURE_FILE "treasure.dat"

typedef struct {
    char id[SIZE];
    char username[SIZE];
    float latitude;
    float longitude;
    char clue[SIZE];
    int value;
} Treasure;

void logAction(char* hunt_id, char* action) {
    char file[SIZE];
    snprintf(file, sizeof(file), "%s/%s", hunt_id, HUNT_FILE);

    int f = open(file, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (f < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        return;
    }

    dprintf(f, "%s\n", action);
    close(f);

    char symlink_name[SIZE];
    snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);
    unlink(symlink_name);
    symlink(file, symlink_name);
}

void addTreasure(char* hunt_id) {
    mkdir(hunt_id, 0755);

    char file[SIZE];
    snprintf(file, sizeof(file), "%s/%s", hunt_id, TREASURE_FILE);

    int f = open(file, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (f < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        return;
    }

    Treasure t;

    printf("Treasure ID: ");
    fgets(t.id, SIZE, stdin);
    t.id[strcspn(t.id, "\n")] = '\0';

    printf("Username: ");
    fgets(t.username, SIZE, stdin);
    t.username[strcspn(t.username, "\n")] = '\0';

    printf("Latitude: ");
    scanf("%f", &t.latitude);
    getchar();

    printf("Longitude: ");
    scanf("%f", &t.longitude);
    getchar(); 

    printf("Clue: ");
    fgets(t.clue, SIZE, stdin);
    t.clue[strcspn(t.clue, "\n")] = '\0';

    printf("Value: ");
    scanf("%d", &t.value);
    getchar(); 

    write(f, &t, sizeof(Treasure));
    close(f);

    //char action[STRING_SIZE];
    //snprintf(action, sizeof(action), "Added treasure %s.", t.id);
    logAction(hunt_id, "Treasure added");

    printf("Treasure added successfully.\n");
}

void viewTreasure(char* hunt_id, char* target_id) {
    char file[SIZE];
    snprintf(file, sizeof(file), "%s/%s", hunt_id, TREASURE_FILE);

    int f = open(file, O_RDONLY);
    if (f < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        return;
    }

    Treasure t;
    int found = 0;

    while (read(f, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        // Debugging: afișează ID-ul citit și cel căutat
        printf("Searching for treasure ID: %s\n", target_id);
        printf("Found treasure ID: %s\n", t.id);

        if (strcmp(t.id, target_id) == 0) {
            printf("ID: %s, Username: %s, Latitude: %.2f, Longitude: %.2f, Value: %d\n",
                t.id, t.username, t.latitude, t.longitude, t.value);
            found = 1;
            break;
        }
    }

    close(f);

    if (!found) {
        printf("Treasure %s not found.\n", target_id);
    }

    char action[SIZE];
    snprintf(action, sizeof(action), "Viewed treasure %s.", target_id);
    logAction(hunt_id, action);
}

void removeTreasure(char* hunt_id, char* target_id) {
    char file[SIZE];
    snprintf(file, sizeof(file), "%s/%s", hunt_id, TREASURE_FILE);

    int f = open(file, O_RDONLY);
    if (f < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        return;
    }

    char temp[SIZE];
    snprintf(temp, sizeof(temp), "%s/temp.dat", hunt_id);

    int temp_f = open(temp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_f < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        close(f);
        return;
    }

    Treasure t;
    int removed = 0;

    while (read(f, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(t.id, target_id) != 0) {
            write(temp_f, &t, sizeof(Treasure));
        } else {
            removed = 1;
        }
    }

    close(f);
    close(temp_f);

    if (removed) {
        rename(temp, file);
        char action[SIZE];
        snprintf(action, sizeof(action), "Removed treasure %s.", target_id);
        logAction(hunt_id, action);
        printf("Treasure %s removed successfully.\n", target_id);
    } else {
        unlink(temp);
        printf("Treasure %s not found.\n", target_id);
    }
}

void listTreasures(char* hunt_id) {
    char file[SIZE];
    snprintf(file, sizeof(file), "%s/%s", hunt_id, TREASURE_FILE);

    struct stat st;
    if (stat(file, &st) < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        return;
    }

    printf("Hunt: %s\n", hunt_id);
    printf("Size: %ld bytes\n", st.st_size);
    printf("Last modified: %s\n", ctime(&st.st_mtime));

    int f = open(file, O_RDONLY);
    if (f < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        return;
    }

    Treasure treasure;
    while (read(f, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %s, User: %s, Lat: %.2f, Long: %.2f, Value: %d\n",
            treasure.id, treasure.username, treasure.latitude, treasure.longitude, treasure.value);
    }
    close(f);

    char action[SIZE];
    snprintf(action, sizeof(action), "Listed treasures from hunt %s.", hunt_id);
    logAction(hunt_id, action);
}

void removeHunt(char* hunt_id) {
    char file[SIZE];
    snprintf(file, sizeof(file), "%s/%s", hunt_id, TREASURE_FILE);
    unlink(file);

    snprintf(file, sizeof(file), "%s/%s", hunt_id, HUNT_FILE);
    unlink(file);

    rmdir(hunt_id);

    char symlink_name[SIZE];
    snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);
    unlink(symlink_name);

    printf("Hunt %s removed.\n", hunt_id);
}



void printUsage(char* arg) {
    printf("Usage:\n");
    printf("%s --add <hunt_id>\n", arg);
    printf("%s --list <hunt_id>\n", arg);
    printf("%s --view <hunt_id> <treasure_id>\n", arg);
    printf("%s --remove_treasure <hunt_id> <treasure_id>\n", arg);
    printf("%s --remove_hunt <hunt_id>\n", arg);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--add") == 0) {
        addTreasure(argv[2]);
    } else if (strcmp(argv[1], "--list") == 0) {
        listTreasures(argv[2]);
    } else if (strcmp(argv[1], "--view") == 0 && argc >= 4) {
        viewTreasure(argv[2], argv[3]);
    } else if (strcmp(argv[1], "--remove_treasure") == 0 && argc >= 4) {
        removeTreasure(argv[2], argv[3]);
    } else if (strcmp(argv[1], "--remove_hunt") == 0) {
        removeHunt(argv[2]);
    } else {
        printUsage(argv[0]);
    }

    return 0;
}
