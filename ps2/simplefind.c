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
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/sysmacros.h> 

//Friendly inode translation to human letters  (holy this took forever to write out prof)
static void mode_translate(mode_t mode, char *output) {
    //different filetypes and corresponding letter
    if (S_ISREG(mode)) {
        output[0] = '-';
    } else if (S_ISDIR(mode)) {
        output[0] = 'd';
    } else if (S_ISLNK(mode)) {
        output[0] = 'l';
    } else if (S_ISCHR(mode)) {
        output[0] = 'c';
    } else if (S_ISBLK(mode)) {
        output[0] = 'b';
    } else if (S_ISSOCK(mode)) {
        output[0] = 's';
    } else if (S_ISFIFO(mode)) {
        output[0] = 'p';
    } else {
        output[0] = '?';
    }

    //File owner permissions (this might be a dumb implementation but its simple)
    //bitwise AND for mask st_mode with octal constants to check permissions
    if (mode & S_IRUSR) { 
        output[1] = 'r';
    } else {
        output[1] = '-';
    }
    if (mode & S_IWUSR) {
        output[2] = 'w';
    } else {
        output[2] = '-';
    }
    if (mode & S_IXUSR) {
        output[3] = 'x';
    } else {
        output[3] = '-';
    }
    //Group permissions
    if (mode & S_IRGRP) {
        output[4] = 'r';
    } else {
        output[4] = '-';
    }
    if (mode & S_IWGRP) {
        output[5] = 'w';
    } else {
        output[5] = '-';
    }
    if (mode & S_IXGRP) {
        output[6] = 'x';
    } else {
        output[6] = '-';
    }
    //Other permissions
    if (mode & S_IROTH) {
        output[7] = 'r';
    } else {
        output[7] = '-';
    }
    if (mode & S_IWOTH) {
        output[8] = 'w';
    } else {
        output[8] = '-';
    }
    if (mode & S_IXOTH) {
        output[9] = 'x';
    } else {
        output[9] = '-';
    }
    output[10] = '\0';  //ends with null terminator
}

void long_print(const char *path, const struct stat *st) {
    //Inode Number
    int i_num = st->st_ino;

    //Blocks (1K block size)
    int block = st->st_blocks / 2;

    //Inode Mode
    char mode[11]; //10 chars needed for mode +1 for null terminator (0 to 10 = 11 spaces)
    mode_translate(st->st_mode, mode);

    //Link Count
    int nlink = st->st_nlink;
    
    //Owner of node -- cool snprintf thanks aidan!
    char owner[64];
    struct passwd *pw = getpwuid(st->st_uid);
    if (pw != NULL) {
        snprintf(owner, sizeof(owner), "%s", pw->pw_name);
    } else {
        snprintf(owner, sizeof(owner), "%u", st->st_uid);
    }

    //Group 
    char group[64];
    struct group *gr = getgrgid(st->st_gid);
    if (gr != NULL) {
        snprintf(group, sizeof(group), "%s", gr->gr_name);
    } else {
        snprintf(group, sizeof(group), "%u", st->st_gid);
    }

    //Size of node - check if char device or block device
    char sizebuf[64];
    if (S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode)) {
        snprintf(sizebuf, sizeof(sizebuf), "%d,%d", major(st->st_rdev), minor(st->st_rdev));
    } else {
        snprintf(sizebuf, sizeof(sizebuf), "%ld", st->st_size);
    }

    //mtime of node (local timezone)
    char time[64];
    struct tm *local_time = localtime(&st->st_mtime);
    strftime(time, sizeof(time), "%b %e %H:%M", local_time);

    //Symlink
    char link[4096];
    if (S_ISLNK(st->st_mode)) {
        ssize_t len = readlink(path, link, sizeof(link) - 1);
        if (len == -1) {
            fprintf(stderr, "Cannot resolve symlink '%s': %s\n", path, strerror(errno));
        }else{
            link[len] = '\0';
            printf(" -> %s", link);
        }  
    }

    if (S_ISLNK(st->st_mode) && link[0] != '\0') { //if symlink, use this print 
    printf("%d %d %s %d %s %s %s %s %s -> %s\n", i_num, block, mode, nlink, owner, group, sizebuf, time, path, link);
    } else {
        printf("%d %d %s %d %s %s %s %s %s\n", i_num, block, mode, nlink, owner, group, sizebuf, time, path);
    }
}

bool match_pattern(const char *path, const char *pattern) {
    if (pattern == NULL) {
        return true;
    }
    const char *base = strrchr(path, '/'); //isolate filename
    if (base != NULL) {
        base = base + 1; //now base points to the filename part
    } else {
        base = path; //no slash in path so use the whole string for file
    }
    return fnmatch(pattern, base, 0) == 0; //fnmatch the goat!
}

int simplefind(const char *path, const char *pattern, bool verbose, bool xdev, dev_t root_dev){
    struct stat st;
    if (lstat(path, &st) == -1) {
        fprintf(stderr, "Cannot stat entry at '%s' while scanning: %s\n", path, strerror(errno));
        return -1;
    }

    //If pattern matches, then print requested verbose or path
    if (match_pattern(path, pattern)) {
        if (verbose){
            long_print(path, &st);
        }else 
            puts(path);
    }

    //Check if st is directory or file (then stop)
    if (!S_ISDIR(st.st_mode)){
        return 0;
    }

    //Enforce -x flag
    if (xdev && (st.st_dev != root_dev)){
        return 0;
    }

    //Open directory for reading
    DIR *d = opendir(path);
    if (d == NULL) {
        fprintf(stderr, "Cannot open directory %s, skipping! %s\n", path, strerror(errno));
        return 0;
    }

    struct dirent *de;
    while ((de = readdir(d)) != NULL) {

        //skip current self and parent directories
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0){ 
            continue;
        }

        char tmp_path[4096]; //append each iteration to path 
        snprintf(tmp_path, sizeof(tmp_path), "%s/%s", path, de->d_name);

        simplefind(tmp_path, pattern, verbose, xdev, root_dev);
    }

    closedir(d);
    return 0;
}

int main(int argc, char *argv[]) {
    bool verbose = false; //long listing format
    bool xdev = false; //x flag
    const char *pattern = NULL;

    //argument parsing logic
    int opt;
    while ((opt = getopt(argc, argv, "lxn:")) != -1) {
        switch (opt) {
            case 'l': 
                verbose = true; 
                break;
            case 'x': 
                xdev = true; 
                break;
            case 'n': 
                pattern = optarg; 
                break;
            default:
                fprintf(stderr, "To run: ./simplefind [-l] [-x] [-n pattern] [starting_path]\n"); //please run only this format otherwise funny stuff may happen
                return 1;
        }
    }

    //Start path defaults to . if not specified
    const char *start;
    if (optind < argc) {
        start = argv[optind];
    } else {
        start = ".";
    }

    struct stat st;
    dev_t root_dev = st.st_dev;

    if (lstat(start, &st) == -1) {
            fprintf(stderr, "Invalid starting path '%s': %s\n", start, strerror(errno));
        return 1;
    }

    return (simplefind(start, pattern, verbose, xdev, root_dev) == -1);
}
