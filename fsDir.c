#include "fsDir.h"
#include "fsLow.h"
#include "fsFree.h"
#include "mfs.h"
#include <stdio.h>
#include <time.h>

DirectoryEntry * rootDir = NULL;
DirectoryEntry rootDirEntry;

// Table row for a directory control block to manage a directory.
// This ensures a directory is only in memory in one place.
typedef struct b_dcb {
    int startingBlock; // serves as unused (-1) or LB position 1+
    DirectoryEntry * dirEntry; // DirectoryEntry for this directory
    DirectoryEntry * dirTable; // Pointer to the first entry _in_ the directory
    int index;
} b_dcb;

#define MAXDCBS 20
b_dcb dcbArray[MAXDCBS];


// Initial a table to hold the directory control blocks
void initialize_dcbArray() {
    // set all the entries to -1 (not used)
    for (int i=0; i<MAXDCBS; i++) {
        dcbArray[i].startingBlock = -1;
        dcbArray[i].index = i;
    }

    // claim the first entry
    dcbArray[0].startingBlock = 0;
}


// find the dcb for a given starting block (which is unique for any given directory)
b_dcb *find_dcb_for_starting_block(int startingBlock) {
    // printf("looking for startingBlock %d\n", startingBlock);
    for(int i=0; i<MAXDCBS; i++) {
        if (dcbArray[i].startingBlock == startingBlock) {
            return &dcbArray[i];
        }
    }
    return NULL;
}


// linear search to find a free spot in the dcbArray
b_dcb *find_free_dcb() {
    for(int i=0; i<MAXDCBS; i++) {
        if (dcbArray[i].startingBlock == -1) {
            return &dcbArray[i];
        }
    }
    return NULL;
}

// find an empty directory entry in a directory table
int find_empty_spot(DirectoryEntry* dir) {
    int num_dir_entries = dir->size / sizeof(DirectoryEntry);
    for(int i = 0; i<num_dir_entries;i++) {
        if(dir[i].type == -1){
            return i;
        }
    }
    return -1;
}


void calc_directory_sizes(uint64_t blockSize, uint64_t *size_in_bytes,
                          uint64_t *size_in_blocks, uint64_t *num_dir_entries) {
    *size_in_bytes = sizeof(DirectoryEntry) * N_DIR_ENTRIES;
    *size_in_blocks = (*size_in_bytes + (blockSize - 1)) / blockSize;

    // for efficiency recalculating how many directory entries can fit in the blocks allocated
    *num_dir_entries = (*size_in_blocks * blockSize) / sizeof(DirectoryEntry);
    *size_in_bytes = *num_dir_entries * sizeof(DirectoryEntry);
}


// Initialize a directory entry
void init_dir_entry(DirectoryEntry *dirent, const char *name, int type,
                    uint64_t position, uint64_t sizeBytes, uint64_t sizeBlocks) {
    dirent->type = type;
    strcpy(dirent->name, name);
    dirent->startingBlock = position;
    dirent->size = sizeBytes;
    dirent->numBlocks = sizeBlocks;
    time(&dirent->createTime);
    dirent->modTime = dirent->createTime;
    dirent->accessTime = dirent->createTime;
}


uint64_t init_directory(DirectoryEntry *parentDirTable,
                        DirectoryEntry *parentDirEntry,
                        const char *name) {
    // We will make a new entry in parentDirTable for the new directory
    // Having done we will save the parent directory using the info from parentDirEntry

    uint64_t size_in_bytes;
    uint64_t size_in_blocks;
    uint64_t num_dir_entries;

    calc_directory_sizes(blockSize, &size_in_bytes, &size_in_blocks, &num_dir_entries);

    // allocate space for directory
    DirectoryEntry *dir = malloc(size_in_blocks * blockSize);
    memset(dir, 0, size_in_blocks * blockSize);

    // set all entries to unused
    for (int i = 0; i < num_dir_entries; i++) {
        dir[i].type = -1;
    }

    // ask free space system for n blocks
    int64_t dir_position = get_free_blocks(size_in_blocks)[0];
    if (dir_position < 0){
        perror("No space to allocate root directory");
        exit(EXIT_FAILURE);
    }

    // create "." directory
    init_dir_entry(&dir[0], ".", FS_TYPE_DIR,
                   dir_position, size_in_bytes, size_in_blocks);

    // create ".." directory
    if (parentDirEntry != NULL) {
        init_dir_entry(&dir[1], "..",
                       FS_TYPE_DIR,
                       parentDirEntry->startingBlock,
                       parentDirEntry->size,
                       parentDirEntry->numBlocks);
    } else {
        // only for root, .. refers to self
        init_dir_entry(&dir[1], "..",
                       FS_TYPE_DIR,
                       dir_position,
                       size_in_bytes,
                       size_in_blocks);
    }

    // insert self into parent directory
    if (parentDirTable != NULL) {
        // find spare DE in parent
        int index = find_empty_spot(parentDirTable);
        if (index == -1) {
            printf("ERROR: no empty spot in parent directory\n");
            return -1;
        }
        // fill in DE
        init_dir_entry(&parentDirTable[index],
                       name, FS_TYPE_DIR,
                       dir_position,
                       size_in_bytes,
                       size_in_blocks);

        // write parent directory to disk
        LBAwrite(parentDirTable,
                 parentDirEntry->numBlocks,
                 parentDirEntry->startingBlock);
    }

    // write dir directory to disk
    LBAwrite(dir, size_in_blocks, dir_position);

    // return the position of the dir directory for VCB
    return dir_position;
}

// initialize the root directory
uint64_t init_root_directory() {
    // initialize directory array
    initialize_dcbArray();

    // return the position of the root directory for VCB
    uint64_t size_in_bytes;
    uint64_t size_in_blocks;
    uint64_t num_dir_entries;

    calc_directory_sizes(blockSize, &size_in_bytes, &size_in_blocks, &num_dir_entries);

    int64_t position = init_directory(NULL, NULL, NULL);
    dcbArray[0].startingBlock = position;

    // fill in fake parent directory entry for root
    init_dir_entry(&rootDirEntry, "/",
                   FS_TYPE_DIR, position,
                   size_in_bytes,
                   size_in_blocks);
    dcbArray[0].dirEntry = &rootDirEntry;

    // read root directory from disk
    rootDir = read_directory(&rootDirEntry);
    dcbArray[0].dirTable = rootDir;

    return position;
}


// read root directory
int64_t read_root_directory(uint64_t position) {
    uint64_t size_in_bytes;
    uint64_t size_in_blocks;
    uint64_t num_dir_entries;
    
    initialize_dcbArray();

    dcbArray[0].startingBlock = position;

    calc_directory_sizes(blockSize, &size_in_bytes, &size_in_blocks, &num_dir_entries);

    // fill in fake parent directory entry for root
    init_dir_entry(&rootDirEntry, "/",
                   FS_TYPE_DIR, position,
                   size_in_bytes,
                   size_in_blocks);

    dcbArray[0].dirEntry = &rootDirEntry;

    // read root directory from disk
    rootDir = read_directory(&rootDirEntry);
    dcbArray[0].dirTable = rootDir;

    return position;
}


DirectoryEntry * read_directory(DirectoryEntry *dirEntry) {
    // check if we already have this directory in cache
    fflush(stdout);
    b_dcb *dcb = find_dcb_for_starting_block(dirEntry->startingBlock);

    if (dcb == NULL) {
        dcb = find_free_dcb();
        if (dcb == NULL) {
            printf("ERROR: no free DCB\n");
            return NULL;
        }

        dcb->startingBlock = dirEntry->startingBlock;
        dcb->dirEntry = dirEntry;
    }

    if (dcb->dirTable == NULL) {
        // printf("reading directory from disk\n");
        dcb->dirTable = malloc(dirEntry->numBlocks * blockSize);
        LBAread(dcb->dirTable, dirEntry->numBlocks, dirEntry->startingBlock);
    }

    return dcb->dirTable;
}


int64_t write_directory(DirectoryEntry *dirEntry, DirectoryEntry *dirTable) {
    // Entry is the directory entry for the directory we are writing
    // Table is the directory table for the directory we are writing
    LBAwrite(dirTable, dirEntry->numBlocks, dirEntry->startingBlock);
    return dirEntry->startingBlock;
}


int64_t update_directory_entry(const char *pathname, DirectoryEntry *fileEntry) {

    char * parentDirPath = get_parent_dir_path(pathname);

    DirectoryEntry *dirEntry = lookup_path(parentDirPath);
    DirectoryEntry *dirTable = read_directory(dirEntry);

    int num_dir_entries = dirEntry->size / sizeof(DirectoryEntry);
    int i;
    for (i = 0; i < num_dir_entries; i++) {
        if (dirTable[i].type != -1 && strcmp(dirTable[i].name, fileEntry->name) == 0) {
            dirTable[i].type = fileEntry->type;
            dirTable[i].size = fileEntry->size;
            dirTable[i].numBlocks = fileEntry->numBlocks;
            dirTable[i].startingBlock = fileEntry->startingBlock;
            dirTable[i].accessTime = fileEntry->accessTime;
            dirTable[i].modTime = fileEntry->modTime;
            //printf("updated directory entry for %s : %d\n", pathname, i);
            break;
        }
    }
    return write_directory(dirEntry, dirTable);
}


// search table of directory entries for a directory or file with the given name
// return the index of the entry if found, NULL otherwise
uint32_t get_index_in_dir(DirectoryEntry *dir, const char *name) {
    if (dir == NULL) {
        return -1;
    }
    int num_dir_entries = dir->size / sizeof(DirectoryEntry);
    for (int i = 0; i < num_dir_entries; i++) {
        if (dir[i].type != -1 && strcmp(dir[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}


// convert a pathname to an absolute pathname
char *all_path_to_abs_path(const char *pathname) {
    int is_root = (strcmp(PWD, "/") == 0);
    int num_sep = is_root ? 0 : 1;
    char *abs_path = NULL;
    abs_path = malloc(strlen(PWD) + strlen(pathname) + 1 + num_sep);
    strcpy(abs_path, PWD);
    if (pathname[0] != '/') {
        // construct new absolute path. append pathname to PWD
        abs_path = malloc(strlen(PWD) + strlen(pathname) + 1 + num_sep);
        strcpy(abs_path, PWD);
        if (!is_root) {
            strcat(abs_path, "/");
        }
        strcat(abs_path, pathname);
    }else{
            abs_path = strdup(pathname);
        }
    return abs_path;
}


// get the directory entry for a given pathname
DirectoryEntry * lookup_name_in_dir(DirectoryEntry *dir, const char *name) {
    if (dir == NULL) {
        return NULL;
    }
    int num_dir_entries = dir->size / sizeof(DirectoryEntry);
    for (int i = 0; i < num_dir_entries; i++) {
        if (dir[i].type != -1 && strcmp(dir[i].name, name) == 0) {
            return &dir[i];
        }
    }
    return NULL;
}

// PWD = /foo/bar
// cd qux
// PWD = /foo/bar/qux
// /
// foo
// bar
// qux


// Directory helper functions
// get the directory entry for a given absolute pathname
DirectoryEntry * lookup_path(const char *pathname) {
    char *abs_pathname = all_path_to_abs_path(pathname);
    // split pathname into tokens
    char *working_pathname = strdup(abs_pathname);
    char *tokens[MAX_NESTED_DIRS];
    char *last_ptr = NULL;
    char *token = strtok_r(working_pathname, "/", &last_ptr);
    int token_count = 0;
    while(token!=NULL) {
        tokens[token_count] = token;
        token_count++;
        token = strtok_r(NULL, "/", &last_ptr);
    }

    // if root, just return root directory entry
    if (token_count == 0) {
        return &rootDirEntry;
    }

    // stack of Directory Entries, the entry not the table
    DirectoryEntry **stack = malloc(sizeof(DirectoryEntry *) * (token_count+1));

    // start at root directory
    stack[0] = read_directory(&rootDirEntry);
    int stack_index = 0;

    // iterate through tokens
    for (int i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], ".") == 0) {
            continue;
        } else if (strcmp(tokens[i], "..") == 0) {
            // if token is "..", pop from stack
            if (stack_index == 0) {
                // if stack is empty, return NULL
                free(stack);
                return &rootDirEntry;
            }
            stack_index--;
            continue;
        } else {
            // if token is not "." or "..", lookup token in current directory
            // if not found, return NULL
            DirectoryEntry *dir = read_directory(stack[stack_index]);
            DirectoryEntry *entry = lookup_name_in_dir(dir, tokens[i]);
            if (entry == NULL) {
                return NULL;
            }
            stack_index++;
            stack[stack_index] = entry;
        }
    }

    DirectoryEntry *result = stack[stack_index];

    free(working_pathname);
    free(stack);

    return result;
}


// This function for finding a file.
DirectoryEntry * lookup_file(const char *pathname) {
    DirectoryEntry *entry = lookup_path(pathname);
    if (entry == NULL) {
        return NULL;
    }
    if (entry->type != FS_TYPE_FILE) {
        printf("returning because entry->type != FS_TYPE_FILE\n");
        return NULL;
    }
    return entry;
}


// find directory entry
DirectoryEntry * lookup_dir(const char *pathname) {
    DirectoryEntry *entry = lookup_path(pathname);
    if (entry == NULL) {
        return NULL;
    }
    if (entry->type != FS_TYPE_DIR) {
        return NULL;
    }
    return entry;
}


char*  get_parent_dir_path(const char * pathname) {
    // "/foo/bar" -> "/foo"
    int len=strlen(pathname);
    int i;

    //get the index where the first slash located
    for (i=len-1; i>=0;i--){
        if(pathname[i] == '/'){
            break;
        }
    }
    if(i < 0){
        return NULL;
    }
    char * parentPath = malloc (i+2);
    strncpy(parentPath,pathname,i+1);
    parentPath[i+1]='\0';
    return parentPath;

}


const char* get_child_dir_name(const char * pathname) {
    // "/foo/bar" -> "bar"

    //get the last part of the path name by phasing the slash
    const char* dirName = strrchr(pathname , '/');

    //if nothing found, return the name back.
    if(dirName == NULL){
        return pathname;
    }
    // +1 for slash
    return dirName +1;
}


// create a new directory
int create_directory(const char * pathname){
    uint64_t size_in_bytes = 0;
    uint64_t size_in_blocks = 0;
    uint64_t num_dir_entries = 0;
    calc_directory_sizes(blockSize,&size_in_bytes,&size_in_blocks,&num_dir_entries);

    char *parentPath = get_parent_dir_path(pathname);
    const char *dirName = get_child_dir_name(pathname);

    if (parentPath == NULL) {
        parentPath = PWD;

    } else if (parentPath[0] != '/') {
        char *path = malloc(strlen(PWD) + strlen(parentPath) + 1);
        strcpy(path, PWD);
        strcat(path, parentPath);
        parentPath = path;
    }

    // find parent directory table
    // and parent directory entry
    DirectoryEntry * parentDirEntry = lookup_dir(parentPath);

    if (parentDirEntry == NULL) {
        printf("parent directory not found 1\n");
        return -1;
    }

    DirectoryEntry * parentDirTable = read_directory(parentDirEntry);

    // check if directory already exists
    if (lookup_name_in_dir(parentDirTable, dirName) != NULL) {
        printf("directory already exists\n");
        return -1;
    }

    init_directory(parentDirTable, parentDirEntry, dirName);

    return 0;
}


DirectoryEntry * create_directory_entry(const char *pathname, int type, int startingBlock,
                                        int numBlocks,int sizeBytes) {
    char *parentPath = get_parent_dir_path(pathname);
    const char *fileName = get_child_dir_name(pathname);
    // find parent directory
    DirectoryEntry *parentDirEntry = lookup_dir(parentPath);
    DirectoryEntry *parentDirTable = read_directory(parentDirEntry);

    int dirIndex = find_empty_spot(parentDirTable);

    if (dirIndex == -1) {
        printf("No empty spot in directory\n");
        return NULL;
    }

    // find an empty directory entry
    DirectoryEntry *dirent = &parentDirTable[dirIndex];
    // initialize directory entry
    init_dir_entry(dirent, fileName, type, startingBlock, sizeBytes, numBlocks);

    write_directory(parentDirEntry, parentDirTable);

    return dirent;
}


int path_check(const char *pathname) {
    char *abs_pathname = all_path_to_abs_path(pathname);
    DirectoryEntry *parentDirTable = NULL;
    DirectoryEntry *parentDirEntry = NULL;
    parentDirEntry = lookup_path(
            get_parent_dir_path(abs_pathname)
    );

    if (parentDirEntry == NULL) {
        printf("parent directory not found.\n");
        return -1;
    }
    parentDirTable = read_directory(parentDirEntry);

    // check if the directory that the caller wants exists or not
    const char *dirName = strrchr(abs_pathname, '/') + 1;
    DirectoryEntry *removed_dir = lookup_name_in_dir(parentDirTable, dirName);
    if (removed_dir == NULL) {
        printf("directory not exists\n");
        return -1;
    }
    return 0;
}


// assumes that the parent directory's DirectoryEntry and its associated directory table are
// already known, and creates a file entry in that directory without performing any lookup
// for the parent directory.
// It provides a lower-level interface to create a file within a specific directory.
DirectoryEntry * create_file_in(const char *pathname,
                                DirectoryEntry *parentDirEntry,
                                DirectoryEntry *parentDirTable) {
    const char *filename = get_child_dir_name(pathname);
    if (lookup_name_in_dir(parentDirTable, filename) != NULL) {
        printf("directory already exists\n");
        return NULL;
    }
    DirectoryEntry *file = create_directory_entry(pathname,
                                                  FS_TYPE_FILE,
                                                  0, 0,0);
    return file;
}


// provides a higher-level interface to create a file base on the pathname.
DirectoryEntry * create_file(const char *pathname) {
    char *parentPath = get_parent_dir_path(pathname);
    const char *dirName = get_child_dir_name(pathname);
    if (parentPath == NULL) {
        parentPath = PWD;
    } else if (parentPath[0] != '/') {
        char *path = malloc(strlen(PWD) + strlen(parentPath) + 1);
        strcpy(path, PWD);
        strcat(path, parentPath);
        parentPath = path;
    }

    // find parent directory table
    // and parent directory entry
    DirectoryEntry * parentDirTable = NULL;
    DirectoryEntry * parentDirEntry = NULL;

    if (strcmp(parentPath, "/") == 0) {

        parentDirTable = read_directory(rootDir);
        parentDirEntry = &rootDirEntry;
    } else {
        parentDirEntry = lookup_dir(parentPath);

        if (parentDirEntry == NULL) {
            printf("parent directory not found\n");
            return NULL;
        }

        parentDirTable = read_directory(parentDirEntry);
    }

    // check if directory already exists
    if (lookup_name_in_dir(parentDirTable, dirName) != NULL) {
        printf("directory already exists\n");
        return NULL;
    }
    DirectoryEntry *file = create_directory_entry(pathname, FS_TYPE_FILE, 0,
                                                  0,0);
    return file;
}


int remove_directory(const char *pathname) {
    // Check if the pathname valid or not
    char *abs_pathname = all_path_to_abs_path(pathname);
    DirectoryEntry *parentDirTable = NULL;
    DirectoryEntry *parentDirEntry = NULL;
    parentDirEntry = lookup_path(
            get_parent_dir_path(abs_pathname)
    );

    if (parentDirEntry == NULL) {
        printf("parent directory not found.\n");
        return -1;
    }
    parentDirTable = read_directory(parentDirEntry);

    // check if the directory that the caller want to remove exists or not
    const char *dirName = strrchr(abs_pathname, '/') + 1;\
    DirectoryEntry *removed_dir = lookup_name_in_dir(parentDirTable, dirName);
    if (removed_dir == NULL) {
        printf("directory not exists.\n");
        return -1;
    }
    if (removed_dir->type == 1) {
        DirectoryEntry* currentDirTable = read_directory(removed_dir);
        if (currentDirTable[2].type != -1) {
            printf("directory is not empty.\n");
            return -1;
        }
    }

    // remove the directory from parents directory entry table
    uint32_t remove_index = get_index_in_dir(parentDirTable, dirName);
    parentDirTable[remove_index].type = -1;
    LBAwrite(parentDirTable,
             parentDirTable->numBlocks,
             parentDirTable->startingBlock);
    if (removed_dir->startingBlock != 0 && removed_dir->numBlocks != 0) {
        free_blocks(removed_dir->numBlocks, removed_dir->startingBlock);
    }

    return 0;
}


int move_file(const char *source, const char *destination) {
    // 1. change to absolute paths
    char *source_abs_pathname = all_path_to_abs_path(source);
    char *destination_abs_pathname = all_path_to_abs_path(destination);
    if (path_check(source_abs_pathname) < 0) {
        printf("source path not found!\n");
        return -1;
    }

    // 2. get source dir/parent
    DirectoryEntry *source_parentDirTable = NULL;
    DirectoryEntry *source_parentDirEntry = NULL;
    source_parentDirEntry = lookup_path(
            get_parent_dir_path(source_abs_pathname)
    );

    if (source_parentDirEntry == NULL) {
        printf("source parent directory not found\n");
        return -1;
    }

    source_parentDirTable = read_directory(source_parentDirEntry);

    DirectoryEntry * source_file = lookup_name_in_dir(source_parentDirTable,
                                                      get_child_dir_name(source_abs_pathname));

    if (source_file == NULL) {
        printf("source file not found\n");
        return -1;
    }

    // 3. get dest dir/parent
    DirectoryEntry *dest_parentDirTable = NULL;
    DirectoryEntry *dest_parentDirEntry = NULL;
    int destIsDir = fs_isDir(destination_abs_pathname);
    char * directoryPath = destIsDir ? destination_abs_pathname :
            get_parent_dir_path(destination_abs_pathname);

    dest_parentDirEntry = lookup_path(
            directoryPath
    );

    if (dest_parentDirEntry == NULL) {
        printf("dest parent directory not found\n");
        return -1;
    }

    dest_parentDirTable = read_directory(dest_parentDirEntry);

    // 3a. does dest file exist?
    DirectoryEntry * dest_file = lookup_name_in_dir(dest_parentDirTable,
                                                    get_child_dir_name(destination_abs_pathname));

    // 3b. is it a directory?
    char * dest_file_path = NULL;
    if (dest_file == NULL && destIsDir) {
        // get the source file name
        // append to dest directory path
        // create new dest file path
        dest_file_path = malloc(strlen(destination_abs_pathname)
                + strlen(get_child_dir_name(source_abs_pathname)) + 2);
        strcpy(dest_file_path, destination_abs_pathname);
        const char * source_filename = get_child_dir_name(source_abs_pathname);
        strcat(dest_file_path, "/");
        strcat(dest_file_path, source_filename);

        destination_abs_pathname = dest_file_path;
        dest_file = lookup_name_in_dir(dest_parentDirTable,source_filename);

    }

    // 3c. create if not existing
    if (dest_file == NULL) {
        dest_file = create_file_in(destination_abs_pathname,
                                   dest_parentDirEntry,
                                   dest_parentDirTable);

        print_dirent(dest_parentDirEntry);

        if (dest_file == NULL) {
            printf("failed to create dest file\n");
            return -1;
        }
    } else {
        // 3d. if existing, check if it's a file
        if (dest_file->type != FS_TYPE_FILE) {
            printf("dest file is not a file\n");
            return -1;
        }

        // release existing blocks
        free_blocks(dest_file->numBlocks, dest_file->startingBlock);
    }

    // 4. copy info from source to dest entries
    dest_file->type = source_file->type;
    dest_file->startingBlock = source_file->startingBlock;
    dest_file->numBlocks = source_file->numBlocks;
    dest_file->size = source_file->size;

    // 5. remove source
    source_file->type = -1;

    // 6. write back directories
    update_directory_entry(source_abs_pathname, source_file);
    update_directory_entry(destination_abs_pathname, dest_file);

    // 7. free anything
    if (dest_file_path != NULL) {
        free(dest_file_path);
    }

    return 0;
}


//*********************** debug print function ***************************************
void print_dirent(DirectoryEntry *dirent) {
    if (dirent == NULL) {
        printf("dirent is null\n");
        return;
    }
//    printf("name: %s, type: %d, startingBlock: %d, numBlocks: %d, size: %d\n",
//    dirent->name, dirent->type, dirent->startingBlock, dirent->numBlocks, dirent->size);
}

void print_dir(DirectoryEntry *dirEntry, DirectoryEntry *dirTable) {
    int num_dir_entries = dirEntry->size / sizeof(DirectoryEntry);
    printf("num_dir_entries: %d\n", num_dir_entries);
    for(int i = 0; i < num_dir_entries; i++) {
        if (dirTable[i].type == -1) {
            continue;
        }
        printf("dirTable[%d]: %p\n", i, &dirTable[i]);
        print_dirent(&dirTable[i]);
  }
}

void print_dcb_array() {
    char time_s[256];
    for(int i = 0; i < MAXDCBS; i++) {
        if (dcbArray[i].startingBlock == -1) {
            continue;
        }
        b_dcb * dcb = &dcbArray[i];
        format_time(dcb->dirEntry->modTime, time_s, 256);
        printf("index: %d, startingBlock: %d, dirEntry: %p, dirTable: %p, name: %s, "
               " mod_time: %s\n",
            i, 
            dcb->startingBlock, 
            dcb->dirEntry,
            dcb->dirTable,
            dcb->dirEntry->name,
            time_s);

        DirectoryEntry *dirent = dcb->dirEntry;
        format_time(dirent->modTime, time_s, 256);
        printf("  --name: %s, type: %d, startingBlock: %d, "
               "numBlocks: %d, size: %d,  mod_time: %s, %p\n",
                dirent->name,
                dirent->type,
                dirent->startingBlock,
                dirent->numBlocks,
                dirent->size,
                time_s,
                dirent);


        int num_dir_entries = dcb->dirEntry->size / sizeof(DirectoryEntry);

        for (int j=0; j < num_dir_entries; j++) {
            if (dcb->dirTable[j].type == -1) {
                continue;
            }
            DirectoryEntry *dirent = &dcb->dirTable[j];
            format_time(dirent->modTime, time_s, 256);
            printf("  name: %s, type: %d, startingBlock: %d,"
                   " numBlocks: %d, size: %d,  mod_time: %s, %p\n",
                    dirent->name,
                    dirent->type,
                    dirent->startingBlock,
                    dirent->numBlocks,
                    dirent->size,
                    time_s,
                    dirent);
        }
    }
}

