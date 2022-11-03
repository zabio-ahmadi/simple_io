#define _GNU_SOURCE
// define _DEFAULT_SOURCE if your glibc version is >= 2.19).

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h> //snprintf
#include <errno.h>
#include <dirent.h>
#include <limits.h> //PATH_MAX
// #define PATH_MAX 255

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

// après les vacances test sur processus

void printDetails(const char *d_name, const char *path, const struct stat file_stat)
{
    if (strcmp(d_name, "..") != 0 && strcmp(d_name, ".") != 0)
    {

        if (S_ISDIR(file_stat.st_mode))
            printf("d");
        else
            printf("-");

        if (file_stat.st_mode & S_IRUSR) // read permission
            printf("r");
        else
            printf("-");

        if (file_stat.st_mode & S_IWUSR) // write permission
            printf("w");
        else
            printf("-");

        if (file_stat.st_mode & S_IXUSR)
            printf("x");
        else
            printf("-");

        if (file_stat.st_mode & S_IRGRP)
            printf("r");
        else
            printf("-");

        if (file_stat.st_mode & S_IWGRP)
            printf("w");
        else
            printf("-");

        if (file_stat.st_mode & S_IXGRP)
            printf("x");
        else
            printf("-");
        if (file_stat.st_mode & S_IROTH)
            printf("r");
        else
            printf("-");

        if (file_stat.st_mode & S_IWOTH)
            printf("w");
        else
            printf("-");

        if (file_stat.st_mode & S_IXOTH)
            printf("x");
        else
            printf("-");

        // affiche la taille de fichier ou dossier
        printf("\t%ld", file_stat.st_size);

        struct tm *timeinfo = localtime(&file_stat.st_mtime); // la date en format local

        char *dateTime = asctime(timeinfo); // string of date  -> Day Mon dd hh:mm:ss yyyy\n

        // remove last trailing character
        if (dateTime[strlen(dateTime) - 1] == '\n')
            dateTime[strlen(dateTime) - 1] = '\0';

        printf("\t %s", dateTime);
        printf("\t%s/%s\n", path, d_name);
    }
}
void readFolder(char *path)
{
    DIR *folder;
    folder = opendir(path);

    if (folder == NULL)
    {
        fprintf(stderr, "can't open folder :%s", path);
        exit(EXIT_FAILURE);
    }

    struct dirent *entry; // contient l'information concernant le répertoire
    const char *d_name;   // nom d'une entrée

    struct stat file_stat;

    while ((entry = readdir(folder)) != NULL) // tante qu'il y a des dossier
    {
        // Obtient le nom de l'entrée et affiche
        d_name = entry->d_name;

        // créer notre prochain path
        char new_path[PATH_MAX];
        snprintf(new_path, PATH_MAX, "%s/%s", path, d_name);

        // lire les meta-données
        stat(new_path, &file_stat);

        printDetails(d_name, path, file_stat); // affiche meta-données de la répertoire.

        if (entry->d_type & DT_DIR)
        {
            if (strcmp(d_name, "..") != 0 && strcmp(d_name, ".") != 0)
                readFolder(new_path);
        }
    }
    if (closedir(folder))
    {
        fprintf(stderr, "Could not close '%s': %s\n",
                path, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{

    readFolder(argv[1]);

    return 0;
}