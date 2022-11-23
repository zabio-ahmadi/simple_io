#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h> //PATH_MAX

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <stdbool.h>

// après les vacances test sur processus

void printDetails(const char *d_name, const char *path, const struct stat file_stat)
{

    // fil_stat conteint les meta-données d'un fichier, dosser
    // st_mode est un champ de bits contenant les permissions et le type d'un inode
    if (S_ISDIR(file_stat.st_mode) && !S_ISLNK(file_stat.st_mode))
        // to do: affichage de liens symbolic
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

    // Les entrées des répertoires sont des liens pointant vers des inodes.
    while ((entry = readdir(folder)) != NULL) // tante qu'il y a des liens
    {
        // Obtient le nom fichier ou dossier
        d_name = entry->d_name;

        // répertoire curent + nome de fichier ou dossier
        char new_path[PATH_MAX];
        snprintf(new_path, PATH_MAX, "%s/%s", path, d_name);

        // lire les meta-données de new_path
        // met le dans structure file_stat

        // error: -1
        // garantir une structure stat
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

mode_t readFileFolderPermission(const char *file)
{

    struct stat file_stat;
    if (lstat(file, &file_stat) == -1)
    {
        fprintf(stderr, "Cannot stat %s: %s\n", file, strerror(errno));
    }

    return file_stat.st_mode;
}

int copy_file_to_file(const char *from, const char *to)
{
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
    {
        fprintf(stderr, "Could not open the file %s: %s\n",
                from, strerror(errno));
        return -1;
    }

    fd_to = open(to, O_WRONLY | O_EXCL, 0600);
    if (fd_to < 0)
    {
        int savedError = errno;
        close(fd_from);
        fprintf(stderr, "Could not open the file %s: %s\n",
                to, strerror(savedError));
        return -1;
    }

    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
        char *out_ptr = buf;
        ssize_t nwritten;
        do
        {
            nwritten = write(fd_to, out_ptr, nread);
            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR)
            {
                int savedError = errno;
                close(fd_from);
                close(fd_to);
                fprintf(stderr, "Could not copy  %s to %s: %s\n",
                        from, to, strerror(savedError));
                return -1;
            }
        } while (nread > 0);
    }

    if (nread != 0)
    {
        int savedError = errno;
        close(fd_from);
        close(fd_to);
        fprintf(stderr, "Could not read %s: %s\n",
                from, strerror(savedError));
        return -1;
    }

    close(fd_from);
    close(fd_to);
    chmod(to, readFileFolderPermission(from));
    return 0;
}
int copy(const char *from, const char *to)
{
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
    {
        fprintf(stderr, "Could not open the file %s: %s\n",
                from, strerror(errno));
        return -1;
    }

    fd_to = open(to, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd_to < 0)
    {
        int savedError = errno;
        close(fd_from);
        fprintf(stderr, "Could not open the file %s: %s\n",
                to, strerror(savedError));
        return -1;
    }

    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
        char *out_ptr = buf;
        ssize_t nwritten;
        do
        {
            nwritten = write(fd_to, out_ptr, nread);
            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR)
            {
                int savedError = errno;
                close(fd_from);
                close(fd_to);
                fprintf(stderr, "Could not copy  %s to %s: %s\n",
                        from, to, strerror(savedError));
                return -1;
            }
        } while (nread > 0);
    }

    if (nread != 0)
    {
        int savedError = errno;
        close(fd_from);
        close(fd_to);
        fprintf(stderr, "Could not read %s: %s\n",
                from, strerror(savedError));
        return -1;
    }

    close(fd_from);
    close(fd_to);
    chmod(to, readFileFolderPermission(from));
    return 0;
}

bool folderExists(const char *name)
{
    DIR *folder;
    folder = opendir(name);

    bool result = false;

    if (folder == NULL)
        return false;
    else
        result = true;

    if (closedir(folder) != 0)
    {
        fprintf(stderr, "Could not close '%s': %s\n", name, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return result;
}

int isFile(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

void copy_folder_to_folder(const char *src, const char *dst)
{

    // structure pour stoquer les données
    struct stat file_stat;

    // garantir une structure stat
    if (lstat(src, &file_stat) == -1)
    {
        fprintf(stderr, "Cannot stat %s: %s\n", src, strerror(errno));
    }

    if (S_ISDIR(file_stat.st_mode) && !S_ISLNK(file_stat.st_mode))
    {

        // copy folder to folder
        DIR *folder;
        folder = opendir(src); // renvoi un flux de type dossier
        if (folder == NULL)
        {
            fprintf(stderr, "can't open folder :%s", src);
            exit(EXIT_FAILURE);
        }

        struct dirent *entry;                     // contient le nome de répertoire + numéro d'inode, type de fichier ...
        const char *d_name;                       // nom d'une entrée
        while ((entry = readdir(folder)) != NULL) // tante qu'il y a des liens
        {
            // Obtient le nom fichier ou dossier
            d_name = entry->d_name;

            // check if destination folder exists
            if (!folderExists(dst))
            {
                if (mkdir(dst, readFileFolderPermission(src)) != 0)
                {
                    fprintf(stderr, "Cannot create dst folder %s: %s\n", dst, strerror(errno));
                }
            }

            // répertoire curent + nome de fichier ou dossier
            char src_new_path[PATH_MAX];
            snprintf(src_new_path, PATH_MAX, "%s/%s", src, d_name);

            char dst_new_path[PATH_MAX];
            snprintf(dst_new_path, PATH_MAX, "%s/%s", dst, d_name);

            if (strcmp(d_name, "..") != 0 && strcmp(d_name, ".") != 0)
            {

                copy_folder_to_folder(src_new_path, dst_new_path);
                if (isFile(src_new_path))
                {
                    if (isFile(dst_new_path))
                    {
                        copy_file_to_file(src_new_path, dst_new_path);
                        // todo: update if src size is bigger than 
                    }
                    else
                    {
                        copy(src_new_path, dst_new_path);
                    }
                }
            }
        }
        if (closedir(folder) != 0)
        {
            fprintf(stderr, "Could not close '%s': %s\n",
                    src, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

void copy_src_dest_single(char *src, char *dst)
{
    if (folderExists(src))
    {
        if (!folderExists(dst))
        {
            if (mkdir(dst, readFileFolderPermission(src)) != 0)
            {
                fprintf(stderr, "Cannot create folder %s: %s\n", dst, strerror(errno));
            }
            else
            {
                char dst_new_path[PATH_MAX];
                snprintf(dst_new_path, PATH_MAX, "%s/%s", dst, src);
                copy_folder_to_folder(src, dst_new_path);
            }
        }
        else
        {
            char dst_new_path[PATH_MAX];
            snprintf(dst_new_path, PATH_MAX, "%s/%s", dst, src);
            copy_folder_to_folder(src, dst_new_path);
        }
    }
    //   if src a file
    else
    {
        // if dst is an existing file
        if (isFile(dst))
        {
            copy_file_to_file(src, dst);
        }
        else
        {
            struct stat file_stat;
            // if dst an already folder is already exist
            if ((stat(dst, &file_stat) == 0) && S_ISDIR(file_stat.st_mode))
            {
                // copy file to directory
                char dst_new_path[PATH_MAX];
                snprintf(dst_new_path, PATH_MAX, "%s/%s", dst, src);
                if (isFile(dst_new_path))
                {
                    copy_file_to_file(src, dst_new_path);
                }
                else
                {
                    copy(src, dst_new_path);
                }
            }
            else
            {
                //  create directory
                mkdir(dst, S_IRUSR | S_IWUSR | S_IXUSR);
                // copy file to directory
                char dst_new_path[PATH_MAX];
                snprintf(dst_new_path, PATH_MAX, "%s/%s", dst, src);
                copy(src, dst_new_path);
            }
        }
    }
}

void copy_src_dest_multiple(int argc, char *argv[])
{
    // because argv[0] is the application name
    int i = 1;
    // destination is the last element
    char *dst = argv[argc - 1];
    while (i <= (argc - 2))
    {
        copy_src_dest_single(argv[i], dst);
        i++;
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

    switch (argc)
    {
    case 2: // affichage
        readFolder(argv[1]);
        break;

    case 3: // copy single
        copy_src_dest_single(argv[1], argv[2]);
        break;
    default:
        copy_src_dest_multiple(argc, argv);
        break;
    }

    return EXIT_SUCCESS;
}