/**************************************************************
* Class:  CSC-415-01 - Spring 2023
* Names: Yan Peng, Zhenyu Lin, Zaijing Liu, Zicheng Tan
* Student IDs: 921759056, 920225329, 921394952, 920261872
* GitHub Name: yuqiao1205
* Group Name: Chinese
* Project: Basic File System
*
* File: mfs.c
*
* Description:  API interfaces
*
**************************************************************/
#include "mfs.h"
#include "fsDir.h"
#include "fsVol.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char *PWD = NULL;

// Directory manipulation functions
char *fs_getcwd(char *pathname, size_t size) {
    if (PWD == NULL) {
        return NULL;
    }

    if (size < strlen(PWD)) {
        return NULL;
    }
    strcpy(pathname, PWD);
    return pathname;
}

char* combine_paths(char *base, const char *filename) {
    char *path = malloc(strlen(base) + strlen(filename) + 2);
    strcpy(path, base);
    strcat(path, "/");
    strcat(path, filename);
    return path;
}

char * absolute_path(const char *pathname) {
    char * composite_path = malloc(sizeof(char) * MAX_PATH_LENGTH);
    // current working directory is in global PWD

    // compute absolute path
    if (pathname[0] == '/') {
        // absolute path
        strcpy(composite_path, pathname);
    } else {
        // relative path
        composite_path = combine_paths(PWD, pathname);
    }

    // tokenize path
    char *tokens[MAX_NESTED_DIRS];
    char *last_ptr = NULL;
    char *token = strtok_r(composite_path, "/", &last_ptr);
    int token_count = 0;
    while(token!=NULL) {
        tokens[token_count] = token;
        token_count++;
        token = strtok_r(NULL, "/", &last_ptr);
    }

    // collapse tokens removing . and ..
    int j = 0;
    for (int i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], ".") == 0) {
            continue;
        } else if (strcmp(tokens[i], "..") == 0) {
            if (j > 0) {
                j--;
            }
        } else {
            tokens[j] = tokens[i];
            j++;
        }
    }
    tokens[j] = NULL;

    // reconstruct path
    char *abs_path = malloc(sizeof(char) * 256);
    strcpy(abs_path, "/");
    for (int i = 0; i < j; i++) {
        strcat(abs_path, tokens[i]);
        // avoid adding trailing slash
        if (i < j - 1) {
            strcat(abs_path, "/");
        }
    }
    free(composite_path);
    return abs_path;
}

int fs_setcwd(char *pathname) {
    // check if absolute or relative path
    char *abs_path = absolute_path(pathname);

    // check if path exists
    DirectoryEntry *dir = lookup_path(abs_path);

    // if path does not exist, return -1, maintain current PWD
    if (dir == NULL) {
        printf("Directory entry does not exist.\n");
        free(abs_path);
        return -1;
    }

    // confirm is directory
    if (dir->type != FS_TYPE_DIR) {
        printf("Directory entry is not file, return error!\n");
        free(abs_path);
        return -1;
    }

    PWD = abs_path;
    return 0;
}

//return 1 if is file, 0 otherwise
int fs_isFile(char *filename) {
    DirectoryEntry *entry = lookup_file(filename);
    return entry != NULL;
}

//return 1 if is directory, 0 otherwise
int fs_isDir(char *pathname) {
    DirectoryEntry *entry = lookup_dir(pathname);
    return entry != NULL;
}

// create directory
int fs_mkdir(const char *pathname, mode_t mode) {
    return create_directory(pathname);
}


fdDir *fs_opendir(const char *pathname) {
    DirectoryEntry *entry = lookup_path(pathname);

    if (entry == NULL) {
        return NULL;
    }

    fdDir *dir = malloc(sizeof(fdDir));
    if (dir == NULL) {
        printf("fail to do malloc");
        return NULL;
    }
    dir->d_reclen = sizeof(fdDir);
    dir->dirEntryPosition = 0;
    dir->directoryStartLocation = entry->startingBlock;
    dir->dirEntry = entry;
    dir->dirTable = read_directory(entry);
    dir->path = absolute_path(pathname);
    return dir;
}


struct fs_diriteminfo *fs_readdir(fdDir *dirp) {
    int i;
    int num_dir_entries = dirp->dirEntry->size / sizeof(DirectoryEntry);
    for (i = dirp->dirEntryPosition; i < num_dir_entries; i++) {
        if (dirp->dirTable[i].type != FS_TYPE_UNUSED) {
            dirp->dirEntryPosition = i;
            break;
        }
    }
    if (i >= num_dir_entries) {
        return NULL;
    }
    struct fs_diriteminfo *item = malloc(sizeof(struct fs_diriteminfo));
    DirectoryEntry *entry = &dirp->dirTable[dirp->dirEntryPosition];
    strcpy(item->d_name, entry->name);
    item->fileType = entry->type;
    item->d_reclen = sizeof(struct fs_diriteminfo);
    dirp->dirEntryPosition++;
    return item;
}

int fs_closedir(fdDir *dirp) {
    free(dirp->path);
}

int fs_stat(const char *path, struct fs_stat *buf) {
    char *abs_path = absolute_path(path);
    DirectoryEntry *entry = lookup_path(abs_path);

    if (entry == NULL) {
        return -1;
    }
    buf->st_size = entry->size;
    buf->st_blocks = entry->numBlocks;
    buf->st_blksize = blockSize;
    buf->st_createtime = entry->createTime;
    buf->st_accesstime = entry->accessTime;
    buf->st_modtime = entry->modTime;
    free(abs_path);
    return 0;
}

int fs_rmdir(const char *pathname) {
    return remove_directory(pathname);
}

int fs_delete(char* filename) {
    return remove_directory(filename);
}

int fs_mv(char* source, char* destination) {
    if (!fs_isFile(source)) {
        printf("Either source does not exist or is directory\n");
        return -1;
    }
    return move_file(source, destination);
}

// ************* file path manipulation functions ***************

const char * fs_getFilename(const char *src) {
    // find the last slash
    int i;
    int last_slash = -1;
    for (i = 0; i < strlen(src); i++) {
        if (src[i] == '/') {
            last_slash = i;
        }
    }
    if (last_slash == -1) {
        return src;
    }

    return strdup(src + last_slash + 1);
}


char * fs_appendFilename(const char * dest, const char * filename) {
    char *combined = malloc(sizeof(char) * 256);
    // treat / specially
    strcpy(combined, dest);
    if (strcmp(dest, "/") != 0) {
        strcat(combined, "/");
    }
    strcat(combined, filename);
    return combined;
}

// ************* utility functions ********************

void format_time(time_t t, char *s, int len)
{
    struct tm *tm = localtime(&t);
    strftime(s, len, "%Y-%m-%d %H:%M", tm);
}
