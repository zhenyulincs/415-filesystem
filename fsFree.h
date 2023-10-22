/**************************************************************
* Class:  CSC-415-01 - Spring 2023
* Names: Yan Peng, Zhenyu Lin, Zaijing Liu, Zicheng Tan
* Student IDs: 921759056, 920225329, 921394952, 920261872
* GitHub Name: yuqiao1205
* Group Name: Chinese
* Project: Basic File System
*
* File: fsFree.h
*
* Description: The header file contains declarations of functions,
*              definitions of data types and
*              macros that are used throughout free map manager
*
**************************************************************/
#include "fsVol.h"

// EXTERNAL API

// initialize free space map
int64_t init_free_space_map(VolumeControlBlock * vcb);

// write free space map to disk
int64_t write_free_space_map();

// write a particular block of free space map to disk
int64_t write_free_space_map_block(uint64_t index);

// read free space map from disk
int64_t read_free_space_map(uint64_t position, uint64_t count);

// find free block, mark it used in map, write map to disk,
// return a table of position for free block
int64_t * get_free_blocks(uint64_t count);

// find a run of count free blocks, return index of start block
int64_t find_free_blocks(uint64_t count);

int64_t * new_get_blocks_for_file(uint64_t startingBlock,
                                  uint64_t currentBlockCount,
                                  uint64_t sizeInBlocks);

// free up blocks at positions
int64_t free_blocks(uint64_t count, uint64_t positions);


// convenience functions for reading and write whole files
int64_t read_file(uint32_t startingBlock, uint64_t countBlocks, void * data);
int64_t write_file(uint32_t startingBlock, uint64_t countBlocks, void * data);

// INTERNAL API

int64_t mark_block_free(uint64_t position);  // mark block free
int64_t mark_block_used(uint64_t position); // mark block used
int64_t is_free(uint64_t index);           // check if the block is free
