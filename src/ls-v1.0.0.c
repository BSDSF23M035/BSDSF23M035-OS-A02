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

// ANSI colors
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[0;34m" // Directory
#define COLOR_GREEN   "\033[0;32m" // Executable
#define COLOR_RED     "\033[0;31m" // Archive
#define COLOR_PINK    "\033[0;35m" // Symlink
#define COLOR_REVERSE "\033[7m"    // Special

// ---------------------- Comparison Function ----------------------
int cmp_str(const void *a, const void *b) {
    char * const *str1 = (char* const*)a;
    char * const *str2 = (char* const*)b;
    return strcmp(*str1, *str2);
}

// ---------------------- Print colored ----------------------
void print_colored(const char *filename, mode_t mode) {
    if (S_ISDIR(mode)) {
        printf(COLOR_BLUE "%s" COLOR_RESET, filename);
    } else if (S_ISLNK(mode)) {
        printf(COLOR_PINK "%s" COLOR_RESET, filename);
    } else if (S_ISREG(mode) && (mode & S_IXUSR)) {
        printf(COLOR_GREEN "%s" COLOR_RESET, filename);
    } else if (strstr(filename, ".tar") || strstr(filename, ".gz") || strstr(filename, ".zip")) {
        printf(COLOR_RED "%s" COLOR_RESET, filename);
    } else if (S_ISCHR(mode) || S_ISBLK(mode) || S_ISFIFO(mode) || S_ISSOCK(mode)) {
        printf(COLOR_REVERSE "%s" COLOR_RESET, filename);
    } else {
        printf("%s", filename); // default
    }
}

// ---------------------- Long Listing ----------------------
void long_listing_from_array(char **filenames, int count) {
    for (int i = 0; i < count; i++) {
        struct stat st;
        if (stat(filenames[i], &st) == -1) continue;

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

        printf("%s %2ld %s %s %5ld %s ",
               perms, (long)st.st_nlink,
               pw ? pw->pw_name : "unknown",
               gr ? gr->gr_name : "unknown",
               (long)st.st_size,
               timebuf);

        print_colored(filenames[i], st.st_mode);
        printf("\n");
    }
}

// ---------------------- Column Display (Down-Then-Across) ----------------------
void column_display_from_array(char **filenames, int count) {
    if (count == 0) return;

    int max_len = 0;
    for (int i = 0; i < count; i++) {
        struct stat st;
        if (stat(filenames[i], &st) == -1) continue;
        if ((int)strlen(filenames[i]) > max_len) max_len = strlen(filenames[i]);
    }

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
            if (idx < count) {
                struct stat st;
                stat(filenames[idx], &st);
                print_colored(filenames[idx], st.st_mode);
                printf("%*s", max_len - (int)strlen(filenames[idx]) + spacing, "");
            }
        }
        printf("\n");
    }
}

// ---------------------- Horizontal Display (-x) ----------------------
void horizontal_display_from_array(char **filenames, int count) {
    if (count == 0) return;

    int max_len = 0;
    for (int i = 0; i < count; i++) {
        struct stat st;
        if (stat(filenames[i], &st) == -1) continue;
        if ((int)strlen(filenames[i]) > max_len) max_len = strlen(filenames[i]);
    }

    struct winsize w;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
        term_width = w.ws_col;

    int spacing = 2;
    int col_width = max_len + spacing;
    int cur_width = 0;

    for (int i = 0; i < count; i++) {
        struct stat st;
        stat(filenames[i], &st);
        print_colored(filenames[i], st.st_mode);
        cur_width += col_width;
        printf("%*s", col_width - (int)strlen(filenames[i]), "");
        if (cur_width + col_width > term_width) {
            printf("\n");
            cur_width = 0;
        }
    }
    if (cur_width != 0) printf("\n");
}

// ---------------------- Read Directory, Sort ----------------------
char **read_and_sort(const char *path, int *count) {
    struct dirent *entry;
    DIR *dir = opendir(path);
    if (!dir) { perror("opendir"); *count = 0; return NULL; }

    char **filenames = NULL;
    *count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue; // skip hidden
        filenames = realloc(filenames, sizeof(char*) * (*count + 1));
        filenames[*count] = strdup(entry->d_name);
        (*count)++;
    }
    closedir(dir);

    if (*count > 0)
        qsort(filenames, *count, sizeof(char*), cmp_str);

    return filenames;
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
    int count;
    char **filenames = read_and_sort(path, &count);
    if (!filenames) return 0;

    switch(mode) {
        case LONG:      long_listing_from_array(filenames, count); break;
        case HORIZONTAL: horizontal_display_from_array(filenames, count); break;
        case DEFAULT:   column_display_from_array(filenames, count); break;
    }

    for (int i = 0; i < count; i++) free(filenames[i]);
    free(filenames);

    return 0;
}
