#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fnmatch.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/sysmacros.h> 

// For LS  
static void format_mode(mode_t mode, char *out) {
    // File type
    if (S_ISREG(mode)) out[0] = '-';
    else if (S_ISDIR(mode)) out[0] = 'd';
    else if (S_ISLNK(mode)) out[0] = 'l';
    else if (S_ISCHR(mode)) out[0] = 'c';
    else if (S_ISBLK(mode)) out[0] = 'b';
    else if (S_ISSOCK(mode)) out[0] = 's';
    else if (S_ISFIFO(mode)) out[0] = 'p';
    else out[0] = '?';

    // Owner permissions
    out[1] = (mode & S_IRUSR) ? 'r' : '-';
    out[2] = (mode & S_IWUSR) ? 'w' : '-';
    out[3] = (mode & S_IXUSR) ? 'x' : '-';

    // Group permissions
    out[4] = (mode & S_IRGRP) ? 'r' : '-';
    out[5] = (mode & S_IWGRP) ? 'w' : '-';
    out[6] = (mode & S_IXGRP) ? 'x' : '-';

    // Other permissions
    out[7] = (mode & S_IROTH) ? 'r' : '-';
    out[8] = (mode & S_IWOTH) ? 'w' : '-';
    out[9] = (mode & S_IXOTH) ? 'x' : '-';
    out[10] = '\0';
}

void print_long(const char *path, const struct stat *st) {
    // 1. inode
    printf("%lu ", (unsigned long) st->st_ino);

    // 2. blocks (512-byte units → 1K blocks)
    printf("%ld ", (long)(st->st_blocks / 2));

    // 3. mode string
    char modebuf[11];
    format_mode(st->st_mode, modebuf);
    printf("%s ", modebuf);

    // 4. link count
    printf("%lu ", (unsigned long) st->st_nlink);

    // 5. owner name or uid
    struct passwd *pw = getpwuid(st->st_uid);
    if (pw) printf("%s ", pw->pw_name);
    else printf("%u ", st->st_uid);

    // 6. group name or gid
    struct group *gr = getgrgid(st->st_gid);
    if (gr) printf("%s ", gr->gr_name);
    else printf("%u ", st->st_gid);

    // 7. size or device major,minor
    if (S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode)) {
        printf("%d,%d ", major(st->st_rdev), minor(st->st_rdev));
    } else {
        printf("%lld ", (long long) st->st_size);
    }

    // 8. mtime
    char timebuf[64];
    struct tm lt;
    localtime_r(&st->st_mtime, &lt);
    strftime(timebuf, sizeof(timebuf), "%b %e %H:%M", &lt);
    printf("%s ", timebuf);

    // 9. path
    printf("%s", path);

    // 10. symlink target
    if (S_ISLNK(st->st_mode)) {
        char linkbuf[4096];
        ssize_t len = readlink(path, linkbuf, sizeof(linkbuf) - 1);
        if (len != -1) {
            linkbuf[len] = '\0';
            printf(" -> %s", linkbuf);
        }
    }

    printf("\n");
}

bool matches_pattern(const char *path, const char *pattern) {
    if (pattern == NULL) {
        return true;
    }
    const char *base = strrchr(path, '/');
    if (base != NULL) {
        base = base + 1;   // now base points to the filename part
    } else {
        base = path;       // no slash in path, so use the whole string
    }
    return fnmatch(pattern, base, 0) == 0;
}

int simplefind(const char *path, const char *pattern,bool verbose, bool xdev, dev_t root_dev){
    struct stat st;
    if (lstat(path, &st) == -1) {
        fprintf(stderr, "lstat(%s): %s\n", path, strerror(errno));
        return -1;
    }

    // Print if pattern matches
    if (matches_pattern(path, pattern)) {
        if (verbose){
            print_long(path, &st);
        }else 
            puts(path);
    }

    // If not a directory, stop here
    if (!S_ISDIR(st.st_mode)) return 0;

    // Enforce -x
    if (xdev && st.st_dev != root_dev) return 0;

    DIR *dirp = opendir(path);
    if (!dirp) {
        fprintf(stderr, "opendir(%s): %s\n", path, strerror(errno));
        return 0;
    }

    struct dirent *de;
    while ((de = readdir(dirp)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;

        char child[4096];
        snprintf(child, sizeof(child), "%s/%s", path, de->d_name);

        simplefind(child, pattern, verbose, xdev, root_dev);
    }

    closedir(dirp);
    return 0;
}

int main(int argc, char *argv[]) {
    bool verbose = false; 
    bool xdev = false;
    const char *pattern = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "lxn:")) != -1) {
        switch (opt) {
            case 'l': verbose = true; 
                break;
            case 'x': xdev = true; 
                break;
            case 'n': pattern = optarg; 
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-x] [-n pattern] [starting_path]\n", argv[0]);
                return 1;
        }
    }

    const char *start = (optind < argc) ? argv[optind] : ".";

    struct stat st;
    if (lstat(start, &st) == -1) {
        perror("lstat");
        return 1;
    }

    dev_t root_dev = st.st_dev;
    if (simplefind(start, pattern, verbose, xdev, root_dev) == -1) {
        return 1;
    } else {
        return 0;
    }
}
