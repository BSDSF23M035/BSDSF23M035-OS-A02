#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <getopt.h>

enum display_mode { DEFAULT, LONG, HORIZONTAL };

// ---------------------- Long Listing (-l) ----------------------
void long_listing(const char *path) {
    (void)path; // to suppress unused warning if path is not used
    struct dirent *entry;
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        struct stat st;
        if (stat(entry->d_name, &st) == -1) {
            perror("stat");
            continue;
        }

        char perms[11];
        perms[0] = S_ISDIR(st.st_mode) ? 'd' : '-';
        perms[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
        perms[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
        perms[3] = (st.st_mode & S_IXUSR) ? 'x' : '-';
        perms[4] = (st.st_mode & S_IRGRP) ? 'r' : '-';
        perms[5] = (st.st_mode & S_IWGRP) ? 'w' : '-';
        perms[6] = (st.st_mode & S_IXGRP) ? 'x' : '-';
        perms[7] = (st.st_mode & S_IROTH) ? 'r' : '-';
        perms[8] = (st.st_mode & S_IWOTH) ? 'w' : '-';
        perms[9] = (st.st_mode & S_IXOTH) ? 'x' : '-';
        perms[10] = '\0';

        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);

        char timebuf[20];
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&st.st_mtime));

        printf("%s %2ld %s %s %5ld %s %s\n",
               perms, (long)st.st_nlink,
               pw ? pw->pw_name : "unknown",
               gr ? gr->gr_name : "unknown",
               (long)st.st_size,
               timebuf,
               entry->d_name);
    }

    closedir(dir);
}

// ---------------------- Column Display (Down-Then-Across) ----------------------
void column_display(const char *path) {
    struct dirent *entry;
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    char **filenames = NULL;
    int count = 0;
    int max_len = 0;

    while ((entry = readdir(dir)) != NULL) {
        filenames = realloc(filenames, sizeof(char*) * (count + 1));
        filenames[count] = strdup(entry->d_name);
        int len = strlen(entry->d_name);
        if (len > max_len) max_len = len;
        count++;
    }
    closedir(dir);

    if (count == 0) return;

    struct winsize w;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
        term_width = w.ws_col;

    int spacing = 2;
    int num_cols = term_width / (max_len + spacing);
    if (num_cols == 0) num_cols = 1;
    int num_rows = (count + num_cols - 1) / num_cols;

    for (int r = 0; r < num_rows; r++) {
        for (int c = 0; c < num_cols; c++) {
            int idx = r + c * num_rows;
            if (idx < count)
                printf("%-*s", max_len + spacing, filenames[idx]);
        }
        printf("\n");
    }

    for (int i = 0; i < count; i++) free(filenames[i]);
    free(filenames);
}

// ---------------------- Horizontal Display (-x) ----------------------
void horizontal_display(const char *path) {
    struct dirent *entry;
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    char **filenames = NULL;
    int count = 0;
    int max_len = 0;

    while ((entry = readdir(dir)) != NULL) {
        filenames = realloc(filenames, sizeof(char*) * (count + 1));
        filenames[count] = strdup(entry->d_name);
        int len = strlen(entry->d_name);
        if (len > max_len) max_len = len;
        count++;
    }
    closedir(dir);

    if (count == 0) return;

    struct winsize w;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
        term_width = w.ws_col;

    int spacing = 2;
    int col_width = max_len + spacing;
    int cur_width = 0;

    for (int i = 0; i < count; i++) {
        printf("%-*s", col_width, filenames[i]);
        cur_width += col_width;
        if (cur_width + col_width > term_width) {
            printf("\n");
            cur_width = 0;
        }
    }
    if (cur_width != 0) printf("\n");

    for (int i = 0; i < count; i++) free(filenames[i]);
    free(filenames);
}

// ---------------------- Main ----------------------
int main(int argc, char *argv[]) {
    int opt;
    enum display_mode mode = DEFAULT;

    while ((opt = getopt(argc, argv, "lx")) != -1) {
        switch(opt) {
            case 'l': mode = LONG; break;
            case 'x': mode = HORIZONTAL; break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-x] [directory]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    const char *path = (optind < argc) ? argv[optind] : ".";

    switch(mode) {
        case LONG:
            long_listing(path);
            break;
        case HORIZONTAL:
            horizontal_display(path);
            break;
        case DEFAULT:
            column_display(path);
            break;
    }

    return 0;
}
