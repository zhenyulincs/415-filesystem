#include <stdlib.h>
#include <time.h>
#include "fsLow.h"

#ifndef FS_DIR_H
#define FS_DIR_H

#define FS_TYPE_FILE 0
#define FS_TYPE_DIR 1
#define FS_TYPE_UNUSED -1

// Directory entry structure for the file system
typedef struct {
    char name[88];       // name of the file or directory
    int type;             // flag indicating whether this is a directory (1) or a file (0) or unused (-1)
    int size;             // size of the file in bytes
    int startingBlock;    // starting location of the file on disk
    int numBlocks;        // number of blocks used by the file
    time_t accessTime;    // time of last access
    time_t modTime;       // time of last modification
    time_t createTime;    // time of last status change
} DirectoryEntry;

#define MAX_NESTED_DIRS 32



#define N_DIR_ENTRIES 20

int create_directory(const char *pathname);
int remove_directory(const char *pathname);
int move_file(const char *source,const char *destination);
uint64_t init_root_directory();
int64_t read_root_directory(uint64_t position);
DirectoryEntry * read_directory(DirectoryEntry *dirEntry);
char*  get_parent_dir_path(const char * pathname);
int64_t update_directory_entry(const char *pathname, DirectoryEntry *fileEntry);
DirectoryEntry * lookup_path(const char *pathname);
DirectoryEntry * lookup_file(const char *pathname);
DirectoryEntry * lookup_dir(const char *pathname);
DirectoryEntry * create_file(const char *pathname);

void print_dirent(DirectoryEntry *dirent);
void print_dcb_array();


#endif
