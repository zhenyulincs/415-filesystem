/**************************************************************
* Class:  CSC-415-01 - Spring 2023
* Names: Yan Peng, Zhenyu Lin, Zaijing Liu, Zicheng Tan
* Student IDs: 921759056, 920225329, 921394952, 920261872
* GitHub Name: yuqiao1205
* Group Name: Chinese
* Project: Basic File System
*
* File: b_io.c
*
* Description: Basic File System - Key File I/O Operations
*
**************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>            // for malloc
#include <string.h>            // for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "fsDir.h"
#include "fsFree.h"
#include "fsVol.h"
#include "b_io.h"
#include "mfs.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

// a struct to hold the information of a file
typedef struct b_fcb {
    char *path; // path of the file
    uint64_t offset; // offset in the file
    char *buf;        //holds the open file buffer
    int64_t block_index; // block index in the file (blocks)
    int index;        //holds the current position in the buffer (bytes)
    int buflen; //holds how many valid bytes are in the buffer (bytes)
    DirectoryEntry *dirent; // a point to directory entry
    int flags; // flags for open
} b_fcb;

b_fcb fcbArray[MAXFCBS];

int startup = 0;    //Indicates that this has not been initialized

//Method to initialize our file system
void b_init() {
    //init fcbArray to all free
    for (int i = 0; i < MAXFCBS; i++) {
        fcbArray[i].buf = NULL; //indicates a free fcbArray
    }

    startup = 1;
}

//Method to get a free FCB element
b_io_fd b_getFCB() {
    for (int i = 0; i < MAXFCBS; i++) {
        if (fcbArray[i].buf == NULL) {
            return i;        //Not thread safe (But do not worry about it for this assignment)
        }
    }
    return (-1);  //all in use
}

// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open(char *filename, int flags) {
    b_io_fd returnFd;

    // Check the flags
    if (!(flags | (O_APPEND | O_TRUNC | O_RDONLY | O_WRONLY | O_RDWR))) {
        printf("Invalid b_open flags: %d!\n", flags);
        return (-1);
    }

    char *abs_path = absolute_path(filename);
    DirectoryEntry *dirent = lookup_file(abs_path);
    if ((dirent == NULL) && (flags & O_CREAT)){
        dirent = create_file(abs_path);
    } else if (dirent == NULL){
        printf("File not found.\n");
        return (-1);
    }

    if (startup == 0) b_init();  //Initialize our system

    returnFd = b_getFCB();                // get our own file descriptor
    // check for error - all used FCB's
    if (returnFd < 0) {
        return (-1);        // all used
    }

    if (dirent == NULL) {
        return (-1);        // file not found
    }

    b_fcb *fcb = &fcbArray[returnFd];
    fcb->path = abs_path;
    fcb->offset = 0;
    fcb->dirent = dirent;
    fcb->block_index = -1;
    fcb->buf = malloc(B_CHUNK_SIZE);
    fcb->index = 0;
    fcb->buflen = 0;
    fcb->flags = flags;

    if (flags & O_APPEND) {      // if append, set offset to end of file
        fcb->offset = fcb->dirent->size;
    }

    if (flags & O_TRUNC) { // if truncate, set size to 0
        fcb->dirent->size = 0;
    }

    return (returnFd);                        // all set
}


// Interface to seek function	
int b_seek(b_io_fd fd, off_t offset, int whence) {
        if (startup == 0) b_init();  //Initialize our system

    // check that fd is between 0 and (MAXFCBS-1)
    if ((fd < 0) || (fd >= MAXFCBS)) {
        return (-1);                    //invalid file descriptor
    }
    b_fcb *fcb = &fcbArray[fd];

    //There are three situations for seek: SEEK_SET, SEEK_CUR, SEEK_END
    if (whence == SEEK_SET){
        //In this situation, the offset is the new position for read
        fcb->offset = offset;
        if (fcb->offset < 0){
            // the read place can't be negative number, because this is the beginning of file
            return (-2);
        }
    }

    if (whence == SEEK_CUR){
        //in this situation, the current read position will add offset
        fcb->offset += offset;
    }

    if (whence == SEEK_END){
        //in this situation, the read position will be the tail of the file and add offset
        fcb->offset = fcb->dirent->size + offset;
    }

    return fcb->offset;
}


// Interface to write function
int b_write(b_io_fd fd, char *buffer, int count) {
    if (startup == 0) b_init();  //Initialize our system

    if (count <= 0) {
        return (-1); // invalid count
    }

    // check that fd is between 0 and (MAXFCBS-1)
    if ((fd < 0) || (fd >= MAXFCBS)) {
        return (-1);                    //invalid file descriptor
    }

    // Locate the fcb
    b_fcb *fcb = &fcbArray[fd];

    // Check if the flag is READ ONLY
    if (fcb->flags & O_RDONLY) {
        printf("b_write error: File is read only.\n");
        return (-1);
    }

    fcb->dirent->accessTime = time(NULL);
    uint64_t fileSize = fcb->offset + count;

    // ask for space
    uint64_t sizeInBlocks = (fileSize + blockSize - 1) / blockSize;
    int64_t *positions = new_get_blocks_for_file(fcb->dirent->startingBlock,
                                                 fcb->dirent->numBlocks,
                                                 sizeInBlocks);
    fcb->dirent->startingBlock = positions[0];
    fcb->dirent->numBlocks = sizeInBlocks;

    // while not written everything
    uint64_t totalToBeWritten = count;
    char *str = buffer;

    while (totalToBeWritten > 0) {
        uint64_t numberToCopy = totalToBeWritten;
        uint64_t spaceAvailable = B_CHUNK_SIZE - fcb->index;

        if (numberToCopy > spaceAvailable) {
            numberToCopy = spaceAvailable;
        }

        int64_t indexIntoBlockPositions = fcb->offset / blockSize;
        int64_t blockPosition = positions[indexIntoBlockPositions];

        // if buffered block is not the correct block, read it
        if (fcb->block_index != blockPosition) {
            LBAread(fcb->buf, 1, blockPosition);
            fcb->block_index = blockPosition;
        }

        // copy data to buffer, update index
        char *nextSpace = &(fcb->buf[fcb->index]);
        memcpy(nextSpace, &str[count-totalToBeWritten], numberToCopy);

        // Perform write
        LBAwrite(fcb->buf, 1, blockPosition);

        // Update the FCB
        fcb->index += numberToCopy;

        // calculate remaining data to be written after this write
        totalToBeWritten -= numberToCopy;

        // Handle the case end of buffer is reached
        if (fcb->index >= (blockSize - 1)) {
            fcb->index = 0;
            fcb->block_index = -1;
        }

        // update offset and file size
        fcb->offset += numberToCopy;
        if (fcb->offset > fcb->dirent->size) {
            fcb->dirent->size = fcb->offset;
        }
    }

    return (0);
}

// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill 
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read(b_io_fd fd, char *buffer, int count) {
    if (startup == 0) b_init();  //Initialize our system

    // check that fd is between 0 and (MAXFCBS-1)
    if ((fd < 0) || (fd >= MAXFCBS)) {
        return (-1);                    //invalid file descriptor
    }

    b_fcb *fcb = &fcbArray[fd];

    if (fcb->flags & O_WRONLY) {
        printf("b_read error: File is write only.\n");
        return (-1);
    }

    // prevent overread;
    if (fcb->offset + count > fcb->dirent->size) {
        count = fcb->dirent->size - fcb->offset;
    }

    // check EOF
    if (fcb->offset >= fcb->dirent->size) {
        return 0;
    }

    int needToBeRead = count;
    int bufferWriteIndex = 0;

    while (needToBeRead > 0) {
        // check if we need to read a new block
        int availableInBuffer = B_CHUNK_SIZE - fcb->index;
        int desireToRead = min(needToBeRead, availableInBuffer);
        int direct = fcb->offset % B_CHUNK_SIZE == 0 && desireToRead >= B_CHUNK_SIZE;

        if (direct) {
            char *readFrom = &(fcb->buf[fcb->index]);
            char *writeTo = &(buffer[bufferWriteIndex]);
            int blockToRead = fcb->dirent->startingBlock + (fcb->offset / B_CHUNK_SIZE);
            int nBlocksToRead = desireToRead / B_CHUNK_SIZE;
            LBAread(readFrom, nBlocksToRead, blockToRead);
            fcb->offset += nBlocksToRead * B_CHUNK_SIZE;
        } else {
            // two cases, something in buffer and buffer empty
            int blockToRead = fcb->dirent->startingBlock + (fcb->offset / B_CHUNK_SIZE);

            // if incorrect block or buffer empty, read from disk
            if (fcb->block_index != blockToRead || fcb->index >= B_CHUNK_SIZE) {
                LBAread(fcb->buf, 1, blockToRead);
                fcb->block_index = blockToRead;
                fcb->index = 0;
            }

            char *readFrom = &(fcb->buf[fcb->index]);
            char *writeTo = &(buffer[bufferWriteIndex]);
            memcpy(writeTo, readFrom, desireToRead);
            fcb->index += desireToRead;
            fcb->offset += desireToRead;
            bufferWriteIndex += desireToRead;
            needToBeRead -= desireToRead;
        }
    }

    fcb->dirent->accessTime = time(NULL);

    return bufferWriteIndex;
}

// Interface to Close the file	
int b_close(b_io_fd fd) {
    if ((fd < 0) || (fd >= MAXFCBS)) {
        return (-1);                    //invalid file descriptor
    }
    b_fcb *fcb = &fcbArray[fd];
    if (fcb->dirent == NULL) {
        return 0;
    }
    fcb->dirent->modTime = time(NULL);
    update_directory_entry(fcb->path, fcb->dirent);
    free(fcb->path);
    free(fcb->buf);
}
