/*
- Programming Assignment 02: ls-v1.2.0 (Column Display - Down Then Across)
- Usage:
*        $ ./bin/ls
*        $ ./bin/ls -l
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

void do_ls(const char *dir, int l_flag);
void print_long_format(const char *path, const char *filename);
void print_column_format(char **files, int count, int max_len);
void mode_to_letters(mode_t mode, char str[]);

int main(int argc, char *argv[])
{
    int opt;
    int l_flag = 0;

    while ((opt = getopt(argc, argv, "l")) != -1)
    {
        switch (opt)
        {
            case 'l':
                l_flag = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [directory...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        do_ls(".", l_flag);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            if (argc - optind > 1)
            {
                printf("%s:\n", argv[i]);
            }
            do_ls(argv[i], l_flag);
            if (i < argc - 1)
            {
                printf("\n");
            }
        }
    }

    return 0;
}

void do_ls(const char *dir, int l_flag)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);

    if (dp == NULL)
    {
        fprintf(stderr, "ls: cannot access '%s': %s\n", dir, strerror(errno));
        return;
    }

    if (l_flag)
    {
        errno = 0;
        while ((entry = readdir(dp)) != NULL)
        {
            if (entry->d_name[0] == '.')
                continue;
            print_long_format(dir, entry->d_name);
        }
    }
    else
    {
        // Feature 3: Store filenames in dynamic array for grid rendering
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

        if (count > 0)
        {
            print_column_format(filenames, count, max_len);

            for (int i = 0; i < count; i++)
            {
                free(filenames[i]);
            }
        }
        free(filenames);
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    closedir(dp);
}

void print_column_format(char **files, int count, int max_len)
{
    struct winsize w;
    int term_width = 80; // Default fallback

    // Get terminal window dimensions
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

    // Render grid down then across
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int idx = r + c * rows;
            if (idx < count)
            {
                if (c == cols - 1 || idx + rows >= count)
                {
                    printf("%s", files[idx]);
                }
                else
                {
                    printf("%-*s", col_width, files[idx]);
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

    printf("%s %2lu %-8s %-8s %8ld %s %s\n",
           mode_str,
           (unsigned long)info.st_nlink,
           owner,
           group,
           (long)info.st_size,
           time_buf,
           filename);
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