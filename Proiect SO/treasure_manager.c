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

void logAction(const char* hunt_id,const char* action) {
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

void addTreasure(const char* hunt_id) {
    mkdir(hunt_id, 0755);

    char file[SIZE];
    snprintf(file, sizeof(file), "%s/%s", hunt_id, TREASURE_FILE);

    int f = open(file, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (f < 0) {
        perror("Eroare deschidere fisier");
        return;
    }

    Treasure treasure;

    printf("Enter the treasure details:\n");

    printf("ID: "); 
    scanf(" %255[^\n]", treasure.id); 
    getchar();

    printf("Username: "); 
    scanf(" %255[^\n]", treasure.username); 
    getchar();

    printf("Latitude: "); 
    scanf("%f", &treasure.latitude); 
    getchar();    

    printf("Longitude: "); 
    scanf("%f", &treasure.longitude); 
    getchar();

    printf("Clue: "); 
    scanf(" %255[^\n]", treasure.clue); 
    getchar();

    printf("Value: "); 
    scanf("%d", &treasure.value); 
    getchar();

    write(f, &treasure, sizeof(Treasure));
    close(f);

    logAction(hunt_id, "Treasure added");

    printf("Treasure added successfully.\n");
}

void viewTreasure(const char* hunt_id,const char* target_id) {
    char file[SIZE];
    snprintf(file, sizeof(file), "%s/%s", hunt_id, TREASURE_FILE);

    int f = open(file, O_RDONLY);
    if (f < 0) {
        perror("Eroare la citire");
        return;
    }

    Treasure treasure;
    int ok = 0;

    for (;;) {
        ssize_t bytes = read(f, &treasure, sizeof(Treasure));
        if (bytes != sizeof(Treasure)) break;

        if (strcmp(treasure.id, target_id) == 0) {
            ok = 1;
            break;
        }
    }

    if (ok) {
        printf(">>> Treasure found:\n");
        printf("ID: %s\nUsername: %s\nLatitude: %.2f\nLongitude: %.2f\nClue: %s\nValue: %d\n",
            treasure.id, treasure.username, treasure.latitude, treasure.longitude, treasure.clue ,treasure.value);
    } else {
        printf("Treasure with ID %s not found.\n", target_id);
    }

    char action[SIZE];
    snprintf(action, sizeof(action), "Viewed treasure %s.", target_id);
    logAction(hunt_id, action);
}

void removeTreasure(const char* hunt_id,const char* target_id) {
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
        printf("Treasure %s deleted successfully.\n", target_id);
    } else {
        unlink(tempFile);
        printf("Treasure %s not found.\n", target_id);
    }
}

void listTreasures(const char* hunt_id) {
    char file[SIZE];
    snprintf(file, sizeof(file), "%s/%s", hunt_id, TREASURE_FILE);

    struct stat st;
    if (stat(file, &st) < 0) {
        perror("Eroare la stat");
        return;
    }

    printf("Hunt: %s\n", hunt_id);
    printf("Size: %ld bytes\n", st.st_size);
    printf("Last update: %s\n", ctime(&st.st_mtime));

    int f = open(file, O_RDONLY);
    if (f < 0) {
        perror("Eroare la deschidere");
        return;
    }

    for(;;)
    {
        Treasure treasure;
        ssize_t bytesRead=read(f,&treasure,sizeof(Treasure));
        if(bytesRead!=sizeof(Treasure)) break;

        printf(">> ID: %s | Username: %s | Lat: %.2f | Long: %.2f | Clue: %s | Value: %d\n",treasure.id,treasure.username,treasure.latitude,treasure.longitude,treasure.clue,treasure.value);
    }

    close(f);

    char action[SIZE];
    snprintf(action, sizeof(action), "Treasures from %s.", hunt_id);
    logAction(hunt_id, action);
}

void removeHunt(const char* hunt_id) {
    char file[SIZE];
    snprintf(file, sizeof(file), "%s/%s", hunt_id, TREASURE_FILE);
    unlink(file);

    snprintf(file, sizeof(file), "%s/%s", hunt_id, HUNT_FILE);
    unlink(file);

    rmdir(hunt_id);

    char symlink_name[SIZE];
    snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);
    unlink(symlink_name);

    printf("Hunt %s deleted.\n", hunt_id);
}



void Help(const char* arg) {
    printf("Utilisare:\n");
    printf("%s --add <hunt_id>\n", arg);
    printf("%s --list <hunt_id>\n", arg);
    printf("%s --view <hunt_id> <treasure_id>\n", arg);
    printf("%s --remove_treasure <hunt_id> <treasure_id>\n", arg);
    printf("%s --remove_hunt <hunt_id>\n", arg);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        Help(argv[0]);
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
        Help(argv[0]);
    }

    return 0;
}
