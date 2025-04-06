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
        perror("Eroare logare");
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
        perror("Eroare deschidere fisier");
        return;
    }

    Treasure treasure;

    printf("Treasure ID: ");
    fgets(treasure.id, SIZE, stdin);
    treasure.id[strcspn(treasure.id, "\n")] = '\0';

    printf("Username: ");
    fgets(treasure.username, SIZE, stdin);
    treasure.username[strcspn(treasure.username, "\n")] = '\0';

    printf("Latitude: ");
    scanf("%f", &treasure.latitude);
    getchar();

    printf("Longitude: ");
    scanf("%f", &treasure.longitude);
    getchar();

    printf("Clue: ");
    fgets(treasure.clue, SIZE, stdin);
    treasure.clue[strcspn(treasure.clue, "\n")] = '\0';

    printf("Value: ");
    scanf("%d", &treasure.value);
    getchar();

    write(f, &treasure, sizeof(Treasure));
    close(f);

    logAction(hunt_id, "Treasure adaugat");

    printf("Treasure adaugat cu succes.\n");
}

void viewTreasure(char* hunt_id, char* target_id) {
    char file[SIZE];
    snprintf(file, sizeof(file), "%s/%s", hunt_id, TREASURE_FILE);

    int f = open(file, O_RDONLY);
    if (f < 0) {
        perror("Eroare la citire");
        return;
    }

    Treasure treasure;
    int found = 0;

    while (read(f, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {

        if (strcmp(treasure.id, target_id) == 0) {
            printf("\n Treasure gasit:\n");
            printf("ID: %s, Username: %s, Latitude: %.2f, Longitude: %.2f, Value: %d\n",
                treasure.id, treasure.username, treasure.latitude, treasure.longitude, treasure.value);
            found = 1;
            break;
        }
    }

    close(f);

    if (!found) {
        printf("Treasure cu ID-ul: %s nu a fost gasit.\n", target_id);
    }

    char action[SIZE];
    snprintf(action, sizeof(action), "Viewed treasure %s.", target_id);
    logAction(hunt_id, action);
}

void removeTreasure(char* hunt_id, char* target_id) {
    char originalFile[SIZE];
    snprintf(originalFile, sizeof(originalFile), "%s/%s", hunt_id, TREASURE_FILE);

    char tempFile[SIZE];
    snprintf(tempFile, sizeof(tempFile), "%s/temp.dat", hunt_id);


    int in_f = open(originalFile, O_RDONLY);
    int out_f = open(tempFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (in_f < 0 || out_f < 0) {
        perror("Eroare la accesarea fișierelor");
        if (in_f >= 0) close(in_f);
        if (out_f >= 0) close(out_f);
        return;
    }

    Treasure treasure;
    int removed = 0;

    while (read(in_f, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(treasure.id, target_id) != 0) {
            write(out_f, &treasure, sizeof(Treasure));
        } else {
            removed = 1;
        }
    }

    close(in_f);
    close(out_f);

    if (removed) {
        rename(tempFile, originalFile);
        char action[SIZE];
        snprintf(action, sizeof(action), "Removed treasure %s.", target_id);
        logAction(hunt_id, action);
        printf("Treasure %s sters cu succes.\n", target_id);
    } else {
        unlink(tempFile);
        printf("Treasure %s nu a fost gasit.\n", target_id);
    }
}

void listTreasures(char* hunt_id) {
    char file[SIZE];
    snprintf(file, sizeof(file), "%s/%s", hunt_id, TREASURE_FILE);

    struct stat st;
    if (stat(file, &st) < 0) {
        perror("Eroare la stat");
        return;
    }

    printf("Hunt: %s\n", hunt_id);
    printf("Dimensiune: %ld bytes\n", st.st_size);
    printf("Ultima modificare: %s\n", ctime(&st.st_mtime));

    int f = open(file, O_RDONLY);
    if (f < 0) {
        perror("Eroare la deschidere");
        return;
    }

    Treasure treasure;
    while (read(f, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %s, User: %s, Lat: %.2f, Long: %.2f, Value: %d\n",
            treasure.id, treasure.username, treasure.latitude, treasure.longitude, treasure.value);
    }
    close(f);

    char action[SIZE];
    snprintf(action, sizeof(action), "Treasure-urile listate din hunt %s.", hunt_id);
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

    printf("Hunt %s a fost sters.\n", hunt_id);
}



void printUsage(char* arg) {
    printf("Utilisare:\n");
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
