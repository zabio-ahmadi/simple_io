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

void readFolder(char *path)
{
    DIR *folder;
    folder = opendir(path);

    if (folder == NULL)
    {
        fprintf(stderr, "can't open folder :%s", path);
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    const char *d_name; // nom d'une entrée
    struct stat file_stat;

    char path2[PATH_MAX];
    while ((entry = readdir(folder)) != NULL)
    {
        // Obtient le nom de l'entrée et affiche
        d_name = entry->d_name;

        snprintf(path2, PATH_MAX, "%s/%s", path, d_name);

        stat(path2, &file_stat);
        if (strcmp(d_name, "..") != 0 && strcmp(d_name, ".") != 0)
        {
            printf((S_ISDIR(file_stat.st_mode)) ? "d" : "-");
            printf((file_stat.st_mode & S_IRUSR) ? "r" : "-");
            printf((file_stat.st_mode & S_IWUSR) ? "w" : "-");
            printf((file_stat.st_mode & S_IXUSR) ? "x" : "-");
            printf((file_stat.st_mode & S_IRGRP) ? "r" : "-");
            printf((file_stat.st_mode & S_IWGRP) ? "w" : "-");
            printf((file_stat.st_mode & S_IXGRP) ? "x" : "-");
            printf((file_stat.st_mode & S_IROTH) ? "r" : "-");
            printf((file_stat.st_mode & S_IWOTH) ? "w" : "-");
            printf((file_stat.st_mode & S_IXOTH) ? "x" : "-");

            printf("\t%ld", file_stat.st_size);

            struct tm *timeinfo = localtime(&file_stat.st_mtime); // or gmtime() depending on what you want
            char *dateTime = asctime(timeinfo);

            // remove last trailing character
            if (dateTime[strlen(dateTime) - 1] == '\n')
                dateTime[strlen(dateTime) - 1] = '\0';

            printf("\t %s", dateTime);
            printf("\t%s/%s\n", path, d_name);
        }

        if (entry->d_type & DT_DIR)
        {

            if (stat(d_name, &file_stat) == 0)
            {

                if (strcmp(d_name, "..") != 0 && strcmp(d_name, ".") != 0)
                {

                    readFolder(path2);
                }
            }
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