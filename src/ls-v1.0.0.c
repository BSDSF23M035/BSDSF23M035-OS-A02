#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <unistd.h>

void column_display(const char *path) {
    struct dirent *entry;
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    // Dynamically store filenames
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

    // Terminal width
    struct winsize w;
    int term_width = 80; // default
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        term_width = w.ws_col;
    }

    int spacing = 2;
    int num_cols = term_width / (max_len + spacing);
    if (num_cols == 0) num_cols = 1;
    int num_rows = (count + num_cols - 1) / num_cols;

    // Print down then across
    for (int r = 0; r < num_rows; r++) {
        for (int c = 0; c < num_cols; c++) {
            int idx = r + c * num_rows;
            if (idx < count) {
                printf("%-*s", max_len + spacing, filenames[idx]);
            }
        }
        printf("\n");
    }

    // Free memory
    for (int i = 0; i < count; i++) free(filenames[i]);
    free(filenames);
}

int main(int argc, char *argv[]) {
    const char *path = (argc > 1) ? argv[1] : ".";
    column_display(path);
    return 0;
}
