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
    // if (strcmp(d_name, "..") != 0 && strcmp(d_name, ".") != 0)
    // {

    // fil_stat conteint les meta-données d'un fichier, dosser
    // st_mode est un champ de bits contenant les permissions et le type d'un inode
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

    if (file_stat.st_mode & S_IXUSR) // execuation permission
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
    // }
}
void readFolder(char *path)
{
    DIR *folder;

    folder = opendir(path); // renvoi un flux de type dossier

    if (folder == NULL)
    {
        fprintf(stderr, "can't open folder :%s", path);
        exit(EXIT_FAILURE);
    }

    // représentent l'organisation des fichiers sous forme d'arborescence
    // sont des inodes dont le contenu est une liste d'entrées dirent
    // Les entrées d'un répertoire sont représentées par la structure:dirent
    struct dirent *entry; // contient le nome de répertoire + numéro d'inode, type de fichier ...
    const char *d_name;   // nom d'une entrée

    struct stat file_stat; // structure pour stoquer les données sur le fichier ou dossier

    /* struct stat
    {
        dev_t st_dev;         // device ID
        ino_t st_ino;         // i-node number
        mode_t st_mode;       // protection and type
        nlink_t st_nlink;     // number of hard links
        uid_t st_uid;         // user ID of owner
        gid_t st_gid;         // group ID of owner
        dev_t st_rdev;        // device type (if special file)
        off_t st_size;        // total size, in bytes
        blksize_t st_blksize; // blocksize for filesystem I/O
        blkcnt_t st_blocks;   // number of 512B blocks
        time_t st_atime;      // time of last access
        time_t st_mtime;      // time of last modification
        time_t st_ctime;      // time of last change
    };
    */

    while ((entry = readdir(folder)) != NULL) // tante qu'il y a des contenus dans ce dossier
    {
        // Obtient le nom fichier ou dossier
        d_name = entry->d_name;

        // répertoire curent + nome de fichier ou dossier
        char new_path[PATH_MAX];
        snprintf(new_path, PATH_MAX, "%s/%s", path, d_name);

        // lire les meta-données de new_path
        // met le dans structure file_stat

        // error: -1
        // garnir une structure stat

        if (stat(new_path, &file_stat) == -1)
        {
            fprintf(stderr, "Cannot stat %s: %s\n", path, strerror(errno));
        }

        printDetails(d_name, path, file_stat); // affiche meta-données de la répertoire.

        // si est un dossier
        // Le champs d_type est un champs de bits contenant des informations sur le type de l'inode associé:
        /*
        DT_DIR	Répertoire
        DT_LNK	Lien symbolique
        DT_REG	Fichier de données
        DT_UNKNOWN	Type inconnu
        */
        if (entry->d_type & DT_DIR)
        {
            // évite le dossier current est le dossier parent
            // ne pas rapeler les deux dossier curent & parent
            // if (strcmp(d_name, "..") != 0 && d_name[0] != '.')
            if (strcmp(d_name, "..") != 0 && strcmp(d_name, ".") != 0)
                readFolder(new_path);
        }
    }
    if (closedir(folder) != 0)
    {
        fprintf(stderr, "Could not close '%s': %s\n",
                path, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        fprintf(stderr, "Err: foldername not specified !\n");
        fprintf(stderr, "usage : ./ultra-cp foldername");
        return EXIT_FAILURE;
    }

    readFolder(argv[1]);

    return EXIT_SUCCESS;
}