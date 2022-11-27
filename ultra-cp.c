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

#include <getopt.h>
#include <err.h>
#include "libgen.h"
#define OPT_PERMS 0b01
#define OPT_LINK 0b10

void printDetails(const char *d_name, const char *path, const struct stat file_stat)
{

    // fil_stat conteint les meta-données d'un fichier, dosser
    // st_mode est un champ de bits contenant les permissions et le type d'un inode
    // to do: affichage de liens symbolic
    if (S_ISDIR(file_stat.st_mode) && !S_ISLNK(file_stat.st_mode))
        printf("d");
    else if (S_ISLNK(file_stat.st_mode))
        printf("l");
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

int isRegularFile(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return (S_ISREG(path_stat.st_mode));
}

bool file_exists(char *filename)
{
    struct stat buffer;
    return (lstat(filename, &buffer) == 0);
}

char *read_link_real_path(char *src)
{
    return realpath(src, NULL);
}

int is_Link(const char *path)
{
    struct stat path_stat;
    lstat(path, &path_stat);
    return (S_ISLNK(path_stat.st_mode));
}

bool directoryExist(char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return (S_ISDIR(path_stat.st_mode));
}

int readFolder(char *src)
{

    DIR *dir = opendir(src); // Assuming absolute pathname here.
    if (dir)
    {

        struct dirent *entry;
        char *d_name;
        while ((entry = readdir(dir)) != NULL)
        {
            // Obtient le nom fichier ou dossier
            d_name = entry->d_name; // Iterates through the entire directory.

            // répertoire curent + nome de fichier ou dossier
            char src_new_path[PATH_MAX];
            snprintf(src_new_path, PATH_MAX, "%s/%s", src, d_name);

            if ((strcmp(d_name, "..") != 0 && strcmp(d_name, ".") != 0) && d_name[0] != '.')
            {
                struct stat info;
                if (!lstat(src_new_path, &info))
                {
                    printDetails(d_name, src, info);
                    // Are we dealing with a directory?
                    if (S_ISDIR(info.st_mode) && !S_ISLNK(info.st_mode))
                    {
                        readFolder(src_new_path);
                    }
                    if (S_ISREG(info.st_mode) || S_ISLNK(info.st_mode))
                    {
                        // printDetails(d_name, src, info);
                    }
                }
            }
        }
    }
    if (closedir(dir) != 0)
    {
        struct stat folderStat;
        lstat(src, &folderStat);
        if ((folderStat.st_mode & S_IRUSR) && (folderStat.st_mode & S_IWUSR) && (folderStat.st_mode & S_IXUSR))
        {
            fprintf(stderr, "Could not close : permission denied '%s': %s\n",
                    src, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    exit(EXIT_SUCCESS);
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

int copy(const char *from, const char *to, bool fileAreadyExist)
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

    // fd_to = open(to, O_WRONLY | O_CREAT | O_EXCL, 777);
    if (fileAreadyExist)
    {
        fd_to = open(to, O_WRONLY | O_EXCL, readFileFolderPermission(from));
    }
    else
    {
        fd_to = open(to, O_WRONLY | O_CREAT | O_EXCL, readFileFolderPermission(from));
    }

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

/* copy recursivly directory */
void rec_copy(char *src, char *dst, bool option_f, bool option_a)
{

    // Assuming Directory already exist
    DIR *dir = opendir(src);
    if (dir == NULL)
    {
        exit(EXIT_FAILURE);
    }

    if (dir)
    {
        struct dirent *entry;
        char *d_name;
        while ((entry = readdir(dir)) != NULL) // Iterates through the entire directory.
        {
            // Obtient le nom fichier ou dossier
            d_name = entry->d_name;

            // répertoire curent + nome de fichier ou dossier
            char src_new_path[PATH_MAX];
            snprintf(src_new_path, PATH_MAX, "%s/%s", src, d_name);

            char dst_new_path[PATH_MAX];
            snprintf(dst_new_path, PATH_MAX, "%s/%s", dst, d_name);
            if ((strcmp(d_name, "..") != 0 && strcmp(d_name, ".") != 0))
            {

                struct stat info;
                if (lstat(src_new_path, &info) == 0)
                {

                    // if directory not exist
                    if (!directoryExist(dst))
                    {
                        mkdir(dst, readFileFolderPermission(src));
                        chmod(dst, readFileFolderPermission(src));
                    }

                    // Are we dealing with a directory?
                    if (S_ISDIR(info.st_mode) && !S_ISLNK(info.st_mode))
                    {
                        // printf("src: %s dst:%s\n", src_new_path, dst_new_path);
                        // Make corresponding directory in the target folder here.
                        rec_copy(src_new_path, dst_new_path, option_f, option_a);
                    }
                    if (S_ISLNK(info.st_mode) && !S_ISDIR(info.st_mode))
                    {
                        // base path
                        char base_path[PATH_MAX];
                        getcwd(base_path, PATH_MAX);

                        char final_dst[PATH_MAX];
                        snprintf(final_dst, 3 * PATH_MAX, "%s/%s", base_path, dst_new_path);

                        // dereference by default which is option f -> false
                        if (option_f == false)
                        {
                            unlink(src_new_path);
                            FILE *fp;
                            fp = fopen(src_new_path, "w");
                            fclose(fp);
                        }
                    }

                    if (S_ISREG(info.st_mode) && info.st_size > 0)
                    {
                        if (isRegularFile(dst_new_path))
                        {
                            // @todo: check
                            struct stat srcStat, dstStat;
                            lstat(src_new_path, &srcStat);
                            lstat(dst_new_path, &dstStat);

                            if (srcStat.st_size > dstStat.st_size)
                            {
                                copy(src_new_path, dst_new_path, true);
                            }
                        }
                        else
                        {
                            copy(src_new_path, dst_new_path, false);
                        }
                    }
                }
            }
        }
    }
    if (closedir(dir) != 0)
    {
        fprintf(stderr, "Could not close directory '%s': %s\n", src, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void copy_src_dest_single(char *src, char *dst, bool option_f, bool option_a)
{
    // copy file to file
    if (isRegularFile(src))
    {
        // printf("regular file\n");

        if (!directoryExist(dst))
        {
            if (file_exists(dst))
            {
                copy(src, dst, true);
            }
            else if (!file_exists(dst))
            {
                copy(src, dst, false);
            }
        }
        else if (directoryExist(dst))
        {
            char dst_new_path[PATH_MAX];
            snprintf(dst_new_path, PATH_MAX, "%s/%s", dst, basename(src));
            if (file_exists(dst_new_path))
            {

                struct stat srcStat, dstStat;
                lstat(src, &srcStat);
                lstat(dst_new_path, &dstStat);

                if (srcStat.st_size > dstStat.st_size)
                {
                    copy(src, dst_new_path, true);
                }
            }
            else if (!file_exists(dst_new_path))
            {
                copy(src, dst_new_path, false);
            }
        }
    }

    else if (directoryExist(src) && !is_Link(src))
    {
        if (!directoryExist(dst))
        {
            printf("destination directory not exists\n");
            exit(EXIT_FAILURE);
        }

        if (directoryExist(dst))
        {
            char dst_new_path[PATH_MAX];
            snprintf(dst_new_path, PATH_MAX, "%s/%s", dst, basename(src));

            if (!directoryExist(dst_new_path))
            {
                mkdir(dst_new_path, readFileFolderPermission(src));
                chmod(dst_new_path, readFileFolderPermission(src));
            }

            rec_copy(src, dst_new_path, option_f, option_a);
        }
    }
    else if (is_Link(src))
    {
        // printf("regular link\n");
        if (!directoryExist(dst))
            exit(EXIT_FAILURE);

        char dst_new_path[PATH_MAX];
        snprintf(dst_new_path, PATH_MAX, "%s/%s", dst, src);
        // printf("src: %s dst : %s\n", read_link_real_path(src), dst_new_path);

        if (symlink(read_link_real_path(src), dst_new_path) != 0)
        {
            fprintf(stderr, "cannot create symoblic link of : %s\n", src);
        }

        // dereference by default which is option f -> false
        if (option_f == false)
        {
            // change to file the symbolic link if option_f is set to false
            unlink(src);
            FILE *fp;
            fp = fopen(src, "w");
            fclose(fp);
        }
    }
}
void copy_src_dest_multiple(int argc, char *argv[], bool option_f, bool option_a)
{
    // because argv[0] is the application name
    int i = 1;
    // destination is the last element
    char *dst = argv[argc - 1];
    while (i <= (argc - 2))
    {
        copy_src_dest_single(argv[i], dst, option_f, option_a);
        i++;
    }
}

int calc_opt(int argc, char *argv[])
{
    static const struct option long_options[] =
        {
            /* name : has_arg : flag : val */
            {"permission ", no_argument, 0, 'a'},
            {"symoblic links ", no_argument, 0, 'f'},
        };
    int opt_number = 0;

    int result;
    int index = -1;

    while ((result = getopt_long(argc, argv, "-a:f:", long_options, &index)) != -1)
    {
        switch (result)
        {
        case 'a':
            opt_number |= OPT_PERMS;

            break;
        case 'f':
            opt_number |= OPT_LINK;
            break;
        case '?':

            exit(EXIT_FAILURE);
        }
    }
    return opt_number;
}

int main(int argc, char *argv[])
{

    int options_id = calc_opt(argc, argv);
    if (options_id == 0)
    {
        // printf("no option \n");

        if (argc < 2)
        {
            fprintf(stderr, "Err: foldername not specified !\n");
            fprintf(stderr, "usage : %s foldername\n", argv[0]);
            return EXIT_FAILURE;
        }

        switch (argc)
        {
        case 2: // affichage
            if (directoryExist(argv[1]))
                readFolder(argv[1]);
            else
                exit(EXIT_FAILURE);

            break;

        case 3: // copy single
            copy_src_dest_single(argv[1], argv[2], false, false);
            // if (file_exists(argv[1]) && file_exists(argv[2]))
            //     copy(argv[1], argv[2], true);

            // if (file_exists(argv[1]) && !file_exists(argv[2]))
            //     copy(argv[1], argv[2], false);

            // else
            //     exit(EXIT_FAILURE);
            break;
        default:
            copy_src_dest_multiple(argc, argv, false, false);
            break;
        }
    }

    // option -f
    else if (options_id == 1 && (strcmp(argv[1], "-f") == 0))
    {
        // printf("only -f with paramters\n");

        if (argc < 3)
        {
            fprintf(stderr, "Err: foldername not specified !\n");
            fprintf(stderr, "usage : ./ultra-cp foldername\n");
            return EXIT_FAILURE;
        }

        switch (argc)
        {
        case 3: // affichage
            readFolder(argv[1]);
            break;

        case 4: // copy single
            copy_src_dest_single(argv[2], argv[3], true, false);
            break;
        default:
            copy_src_dest_multiple(argc, argv, true, false);
            break;
        }
    }
    // option -a
    else if (options_id == 2 && (strcmp(argv[1], "-a") == 0))
    {
        // printf("only -a with paramters\n");
        if (argc < 3)
        {
            fprintf(stderr, "Err: foldername not specified !\n");
            fprintf(stderr, "usage : ./ultra-cp foldername\n");
            return EXIT_FAILURE;
        }

        switch (argc)
        {
        case 3: // affichage
            readFolder(argv[1]);
            break;

        case 4: // copy single
            copy_src_dest_single(argv[2], argv[3], false, true);
            break;
        default:
            copy_src_dest_multiple(argc, argv, false, true);
            break;
        }
    }

    // option mixed
    else
    {
        printf("USAGE [OPTIONS] [PARAMETERS]: \n");
        printf("ONLY ONE OPTION A TIME COULD BE PASSED !\n");
        printf("-a: change permissions\n");
        printf("-f: dereference symbolic links\n");
    }

    return EXIT_SUCCESS;
}