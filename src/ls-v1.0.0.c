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

// ---------------------- Comparison Function for qsort ----------------------
int cmp_str(const void *a, const void *b) {
    char * const *str1 = (char* const*)a;
    char * const *str2 = (char* const*)b;
    return strcmp(*str1, *str2);
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

        printf("%s %2ld %s %s %5ld %s %s\n",
               perms, (long)st.st_nlink,
               pw ? pw->pw_name : "unknown",
               gr ? gr->gr_name : "unknown",
               (long)st.st_size,
               timebuf,
               filenames[i]);
    }
}

// ---------------------- Column Display (Down-Then-Across) ----------------------
void column_display_from_array(char **filenames, int count) {
    if (count == 0) return;

    int max_len = 0;
    for (int i = 0; i < count; i++)
        if ((int)strlen(filenames[i]) > max_len) max_len = strlen(filenames[i]);

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
}

// ---------------------- Horizontal Display (-x) ----------------------
void horizontal_display_from_array(char **filenames, int count) {
    if (count == 0) return;

    int max_len = 0;
    for (int i = 0; i < count; i++)
        if ((int)strlen(filenames[i]) > max_len) max_len = strlen(filenames[i]);

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
}

// ---------------------- Read Directory, Skip Hidden, Sort ----------------------
char **read_and_sort(const char *path, int *count) {
    struct dirent *entry;
    DIR *dir = opendir(path);
    if (!dir) { perror("opendir"); *count = 0; return NULL; }

    char **filenames = NULL;
    *count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue; // skip hidden files
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
