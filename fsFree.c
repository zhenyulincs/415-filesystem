/**************************************************************
* Class:  CSC-415-01 - Spring 2023
* Names: Yan Peng, Zhenyu Lin, Zaijing Liu, Zicheng Tan
* Student IDs: 921759056, 920225329, 921394952, 920261872
* GitHub Name: yuqiao1205
* Group Name: Chinese
* Project: Basic File System
*
* File: fsFree.c
*
* Description: This file for managing free space map and helper functions
*
**************************************************************/

#include "fsVol.h"
#include "fsFree.h"
#include "fsLow.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

u_int8_t * freeSpaceMap = NULL;
uint64_t freeSpaceBlocks;

int64_t init_free_space_map(VolumeControlBlock * vcb) {
    // how many blocks for free space map
    uint64_t freeSpaceBitsPerBlock = vcb->blockSize * 8; // 4096 for 512 byte blocks

    // need to round up
    vcb->freeSpaceBlocks = vcb->numberOfBlocks / freeSpaceBitsPerBlock;
    if (vcb->numberOfBlocks % freeSpaceBitsPerBlock != 0) {
        vcb->freeSpaceBlocks++;
    }

    printf("fsb: %ld\n",  vcb->freeSpaceBlocks);

    vcb->freeSpaceMapPosition = 1;
    freeSpaceBlocks = vcb->freeSpaceBlocks;

    // allocate memory for free space map
    freeSpaceMap = malloc(vcb->freeSpaceBlocks * vcb->blockSize);

    // mark all blocks as free
    memset(freeSpaceMap, 0xff, vcb->freeSpaceBlocks * vcb->blockSize);

    // volume control block and free space map are used
    for (int64_t i = 0; i < 1 + vcb->freeSpaceBlocks; i++) {
        mark_block_used(i);
    }

    write_free_space_map();

    return 0;
}

// write free space map to disk
int64_t write_free_space_map() {
    LBAwrite(freeSpaceMap, freeSpaceBlocks, 1);
}

// write a particular block of free space map to disk
int64_t write_free_space_map_block(uint64_t index) {
    u_int8_t * freeSpaceMapOffset = freeSpaceMap + index * blockSize;
    LBAwrite(freeSpaceMapOffset, 1, 1 + index);
}

// read free space map from disk
int64_t read_free_space_map(uint64_t position, uint64_t count) {
    freeSpaceBlocks = count;
    freeSpaceMap = malloc(freeSpaceBlocks * blockSize);
    LBAread(freeSpaceMap, freeSpaceBlocks, position);
}

// free up blocks at positions
int64_t free_blocks(uint64_t count, uint64_t startingBlock) {
    // mark blocks at positions as free in map, write map to disk
    int64_t free_block_count = 0;

    for (int64_t i=0; i < count; i++) {
        int64_t position = startingBlock + i;
        mark_block_free(position);
        free_block_count += 1;
    }

    write_free_space_map();

    return free_block_count;
}

// find a run of count free blocks, return index of start block
int64_t find_free_blocks(uint64_t count)
{
    int64_t start_block = -1;
    int64_t end_block = -1;
    int64_t map_len = freeSpaceBlocks * blockSize * 8;

    for (int64_t i = 0; i < map_len; i++) {
        if (is_free(i)) {
            if (start_block == -1) {
                start_block = i;
            }
            end_block = i;
            if (end_block - start_block + 1 == count) {
                return start_block;
            }
        } else {
            start_block = -1;
            end_block = -1;
        }
    }

    return -1;
}

// find free block, mark it used in map, write map to disk,
// return a table of position for free block
int64_t * get_free_blocks(uint64_t count)
{
    int64_t * positions = malloc(sizeof(int64_t) * (count + 1));
    int64_t start_block = find_free_blocks(count);

    if (start_block == -1) {
        return NULL;
    }

    // mark blocks as used
    for (int64_t i=0; i < count; i++) {
        int64_t position = start_block + i;
        positions[i] = position;
        mark_block_used(position);
    }

    positions[count] = -1;

    write_free_space_map();

    return positions;
}

// Allocate and return an array of count+1 that represent block positions for a file.
// Assumes that the blocks for the file are contiguous, and if they are not,
// it moves the old blocks to a new location and allocates an extended space for the file.
int64_t * get_blocks_for_file(uint64_t startingBlock, uint64_t currentBlockCount,
                              uint64_t sizeInBlocks) {
    int64_t * positions = malloc(sizeof(int64_t) * (sizeInBlocks + 1));

    for(int64_t i = 0; i < currentBlockCount; i++) {
        uint64_t position = startingBlock + i;
        positions[i] = position;
    }

    // assume contiguous blocks are available, exit otherwise
    int64_t blocksAllocated = 0;
    int noContiguousBlocks = 0;
    for(int64_t i=currentBlockCount; i < sizeInBlocks; i++) {
        uint64_t position = startingBlock + i;
        if (!is_free(position)) {
            printf("Error, no contiguous blocks available for file\n");
            noContiguousBlocks = 1;
        }
        positions[i] = position;
        mark_block_used(position);
        blocksAllocated += 1;
    }

    // deal with case where contiguous blocks are not available
    if (noContiguousBlocks) {
        // find new space
        int64_t * new_positions = get_free_blocks(sizeInBlocks);
        // move old blocks to new space
        char * buffer = malloc(blockSize);
        for(int64_t i = 0; i < currentBlockCount; i++) {
            uint64_t src_position = startingBlock + i;
            uint64_t dest_position = new_positions[i];
            LBAread(buffer, 1, src_position);
            LBAwrite(buffer, 1, dest_position);
        }
        // free old space, buffer and old positions
        free_blocks(currentBlockCount, startingBlock);
        free(buffer);
        free(positions);
        write_free_space_map();
        return new_positions;
    }

    positions[sizeInBlocks] = -1;

    if (blocksAllocated > 0) {
        write_free_space_map();
    }
    return positions;
}


// Get blocks for a file
// Initially no blocks are allocated for the file so a simple
// contiguous range needs to be found. If the file is being extended,
// then the existing contiguous blocks are checked to see if they are available.
// If not the file is relocated to a new contiguous range.
int64_t * new_get_blocks_for_file(uint64_t startingBlock,
                                  uint64_t currentBlockCount,
                                  uint64_t sizeInBlocks) {
    if (currentBlockCount == 0) {
        return get_free_blocks(sizeInBlocks);
    } else {
        return get_blocks_for_file(startingBlock, currentBlockCount, sizeInBlocks);
    }
}


// read the contents of a file from storage blocks into a memory buffer
int64_t read_file(uint32_t startingBlock, uint64_t countBlocks, void * data) {
    int64_t * positions = get_blocks_for_file(startingBlock,
                                              countBlocks,
                                              countBlocks);
    if (positions == NULL) {
        return -1;
    }
    for (int i=0; i < countBlocks; i++) {
        LBAread(data + i * blockSize, 1, positions[i]);
    }
    free(positions);
    return 0;
}


// write the contents of a file from a memory buffer to storage blocks
int64_t write_file(uint32_t startingBlock, uint64_t countBlocks, void * data) {
    int64_t * positions = get_blocks_for_file(startingBlock,
                                              countBlocks,
                                              countBlocks);
    if (positions == NULL) {
        return -1;
    }
    for (int i=0; i < countBlocks; i++) {
        LBAwrite(data + i * blockSize, 1, positions[i]);
    }
    free(positions);
    return 0;
}

// INTERNAL API

int64_t mark_block_free(uint64_t position) {
    int64_t block_index = position / 4096;
    int64_t byte_index = position / 8;
    u_int8_t or_value = 1 << (position % 8);
    freeSpaceMap[byte_index] |= or_value;
    return 0;
}

int64_t mark_block_used(uint64_t position) {
    int64_t block_index = position / 4096;
    int64_t byte_index = position / 8;
    int64_t bit_position = position % 8;

    // 000010000 -> 111101111
    u_int8_t or_value = 1 << bit_position;
    u_int8_t mask_value = ~or_value;

    freeSpaceMap[byte_index] &= mask_value;
    return 0;
}

int64_t is_free(uint64_t index) {
    return freeSpaceMap[index/8] & (1<<(index % 8));
}

