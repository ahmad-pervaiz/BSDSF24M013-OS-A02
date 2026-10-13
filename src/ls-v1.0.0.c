/*
- Programming Assignment 02: ls-v1.6.0 (Recursive Listing -R)
- Usage:
*        $ ./bin/ls -R
*        $ ./bin/ls -l -R
*        $ ./bin/ls -x -R
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

extern int errno;

#define COLOR_RESET   "\033[0m"
#define COLOR_DIR     "\033[1;34m"
#define COLOR_EXEC    "\033[1;32m"
#define COLOR_LNK     "\033[1;36m"
#define COLOR_FIFO    "\033[0;33m"
#define COLOR_SOCK    "\033[1;35m"

void do_ls(const char *dir, int l_flag, int x_flag, int R_flag);
void print_long_format(const char *path, const char *filename);
void print_column_format(const char *dir, char **files, int count, int max_len);
void print_horizontal_format(const char *dir, char **files, int count, int max_len);
void mode_to_letters(mode_t mode, char str[]);
int compare_filenames(const void *a, const void *b);
const char *get_color_code(const char *path, const char *filename);

int main(int argc, char *argv[])
{
    int opt;
    int l_flag = 0;
    int x_flag = 0;
    int R_flag = 0;

    while ((opt = getopt(argc, argv, "lxR")) != -1)
    {
        switch (opt)
        {
            case 'l':
                l_flag = 1;
                break;
            case 'x':
                x_flag = 1;
                break;
            case 'R':
                R_flag = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-x] [-R] [directory...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        do_ls(".", l_flag, x_flag, R_flag);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            if (argc - optind > 1)
            {
                printf("%s:\n", argv[i]);
            }
            do_ls(argv[i], l_flag, x_flag, R_flag);
            if (i < argc - 1)
            {
                printf("\n");
            }
        }
    }

    return 0;
}

int compare_filenames(const void *a, const void *b)
{
    const char *str1 = *(const char **)a;
    const char *str2 = *(const char **)b;
    return strcmp(str1, str2);
}

const char *get_color_code(const char *path, const char *filename)
{
    struct stat info;
    char fullpath[1024];

    snprintf(fullpath, sizeof(fullpath), "%s/%s", path, filename);

    if (lstat(fullpath, &info) == -1)
    {
        return COLOR_RESET;
    }

    if (S_ISDIR(info.st_mode))  return COLOR_DIR;
    if (S_ISLNK(info.st_mode))  return COLOR_LNK;
    if (S_ISFIFO(info.st_mode)) return COLOR_FIFO;
    if (S_ISSOCK(info.st_mode)) return COLOR_SOCK;
    if (info.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) return COLOR_EXEC;

    return COLOR_RESET;
}

void do_ls(const char *dir, int l_flag, int x_flag, int R_flag)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);

    if (dp == NULL)
    {
        fprintf(stderr, "ls: cannot access '%s': %s\n", dir, strerror(errno));
        return;
    }

    int capacity = 100;
    int count = 0;
    int max_len = 0;
    char **filenames = malloc(capacity * sizeof(char *));

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        int len = strlen(entry->d_name);
        if (len > max_len)
            max_len = len;

        if (count >= capacity)
        {
            capacity *= 2;
            filenames = realloc(filenames, capacity * sizeof(char *));
        }

        filenames[count] = strdup(entry->d_name);
        count++;
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    closedir(dp);

    if (count > 0)
    {
        qsort(filenames, count, sizeof(char *), compare_filenames);

        if (l_flag)
        {
            for (int i = 0; i < count; i++)
            {
                print_long_format(dir, filenames[i]);
            }
        }
        else if (x_flag)
        {
            print_horizontal_format(dir, filenames, count, max_len);
        }
        else
        {
            print_column_format(dir, filenames, count, max_len);
        }

        // Feature 7: Recursive Directory Traversal
        if (R_flag)
        {
            for (int i = 0; i < count; i++)
            {
                struct stat info;
                char fullpath[1024];

                snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, filenames[i]);

                if (lstat(fullpath, &info) == 0 && S_ISDIR(info.st_mode))
                {
                    printf("\n%s:\n", fullpath);
                    do_ls(fullpath, l_flag, x_flag, R_flag);
                }
            }
        }

        for (int i = 0; i < count; i++)
        {
            free(filenames[i]);
        }
    }

    free(filenames);
}

void print_horizontal_format(const char *dir, char **files, int count, int max_len)
{
    struct winsize w;
    int term_width = 80;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
    {
        term_width = w.ws_col;
    }

    int spacing = 2;
    int col_width = max_len + spacing;
    int cols = term_width / col_width;

    if (cols < 1)
        cols = 1;

    for (int i = 0; i < count; i++)
    {
        const char *color = get_color_code(dir, files[i]);
        int pad = col_width - (int)strlen(files[i]);

        if ((i + 1) % cols == 0 || i == count - 1)
        {
            printf("%s%s%s\n", color, files[i], COLOR_RESET);
        }
        else
        {
            printf("%s%s%s%*s", color, files[i], COLOR_RESET, pad, "");
        }
    }
}

void print_column_format(const char *dir, char **files, int count, int max_len)
{
    struct winsize w;
    int term_width = 80;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
    {
        term_width = w.ws_col;
    }

    int spacing = 2;
    int col_width = max_len + spacing;
    int cols = term_width / col_width;

    if (cols < 1)
        cols = 1;

    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int idx = r + c * rows;
            if (idx < count)
            {
                const char *color = get_color_code(dir, files[idx]);
                int pad = col_width - (int)strlen(files[idx]);

                if (c == cols - 1 || idx + rows >= count)
                {
                    printf("%s%s%s", color, files[idx], COLOR_RESET);
                }
                else
                {
                    printf("%s%s%s%*s", color, files[idx], COLOR_RESET, pad, "");
                }
            }
        }
        printf("\n");
    }
}

void print_long_format(const char *path, const char *filename)
{
    struct stat info;
    char fullpath[1024];

    snprintf(fullpath, sizeof(fullpath), "%s/%s", path, filename);

    if (lstat(fullpath, &info) == -1)
    {
        perror("lstat error");
        return;
    }

    char mode_str[11];
    mode_to_letters(info.st_mode, mode_str);

    struct passwd *pw = getpwuid(info.st_uid);
    char *owner = (pw != NULL) ? pw->pw_name : "UNKNOWN";

    struct group *gr = getgrgid(info.st_gid);
    char *group = (gr != NULL) ? gr->gr_name : "UNKNOWN";

    char time_buf[64];
    struct tm *time_info = localtime(&info.st_mtime);
    strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", time_info);

    const char *color = get_color_code(path, filename);

    printf("%s %2lu %-8s %-8s %8ld %s %s%s%s\n",
           mode_str,
           (unsigned long)info.st_nlink,
           owner,
           group,
           (long)info.st_size,
           time_buf,
           color,
           filename,
           COLOR_RESET);
}

void mode_to_letters(mode_t mode, char str[])
{
    strcpy(str, "----------");

    if (S_ISDIR(mode))  str[0] = 'd';
    if (S_ISCHR(mode))  str[0] = 'c';
    if (S_ISBLK(mode))  str[0] = 'b';
    if (S_ISLNK(mode))  str[0] = 'l';
    if (S_ISFIFO(mode)) str[0] = 'p';
    if (S_ISSOCK(mode)) str[0] = 's';

    if (mode & S_IRUSR) str[1] = 'r';
    if (mode & S_IWUSR) str[2] = 'w';
    if (mode & S_IXUSR) str[3] = 'x';

    if (mode & S_IRGRP) str[4] = 'r';
    if (mode & S_IWGRP) str[5] = 'w';
    if (mode & S_IXGRP) str[6] = 'x';

    if (mode & S_IROTH) str[7] = 'r';
    if (mode & S_IWOTH) str[8] = 'w';
    if (mode & S_IXOTH) str[9] = 'x';
}