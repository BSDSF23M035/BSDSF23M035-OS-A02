#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

void print_colored(const char *path, const char *name);
void list_directory(const char *dirname, int recursive);
int compare_names(const void *a, const void *b);
int is_directory(const char *path);

int main(int argc, char *argv[]) {
    int recursive = 0;
    int opt;

    // Parse command-line flags
    while ((opt = getopt(argc, argv, "R")) != -1) {
        switch (opt) {
            case 'R':
                recursive = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-R] [directory]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    const char *path = (optind < argc) ? argv[optind] : ".";
    list_directory(path, recursive);

    return 0;
}

// Comparison function for qsort
int compare_names(const void *a, const void *b) {
    const char *s1 = *(const char **)a;
    const char *s2 = *(const char **)b;
    return strcmp(s1, s2);
}

// Check if path is directory
int is_directory(const char *path) {
    struct stat st;
    if (lstat(path, &st) == -1) return 0;
    return S_ISDIR(st.st_mode);
}

// Print with color based on file type
void print_colored(const char *path, const char *name) {
    struct stat st;
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", path, name);

    if (lstat(fullpath, &st) == -1) {
        perror("stat");
        return;
    }

    // Directory = Blue
    if (S_ISDIR(st.st_mode))
        printf("\033[1;34m%s\033[0m\n", name);
    // Executable = Green
    else if (st.st_mode & S_IXUSR)
        printf("\033[1;32m%s\033[0m\n", name);
    // Symlink = Pink
    else if (S_ISLNK(st.st_mode))
        printf("\033[1;35m%s\033[0m\n", name);
    // Archive = Red
    else if (strstr(name, ".tar") || strstr(name, ".gz") || strstr(name, ".zip"))
        printf("\033[1;31m%s\033[0m\n", name);
    else
        printf("%s\n", name);
}

// Core recursive listing
void list_directory(const char *dirname, int recursive) {
    DIR *dir = opendir(dirname);
    if (!dir) {
        perror(dirname);
        return;
    }

    struct dirent *entry;
    char **entries = NULL;
    int count = 0;

    // Read entries
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        entries = realloc(entries, sizeof(char *) * (count + 1));
        entries[count] = strdup(entry->d_name);
        count++;
    }
    closedir(dir);

    // Sort alphabetically
    qsort(entries, count, sizeof(char *), compare_names);

    printf("\n%s:\n", dirname);
    for (int i = 0; i < count; i++) {
        print_colored(dirname, entries[i]);
    }

    // Recursively list subdirectories
    if (recursive) {
        for (int i = 0; i < count; i++) {
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", dirname, entries[i]);
            if (is_directory(path)) {
                list_directory(path, recursive);
            }
        }
    }

    // Free memory
    for (int i = 0; i < count; i++)
        free(entries[i]);
    free(entries);
}
