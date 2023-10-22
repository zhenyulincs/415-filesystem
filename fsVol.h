/**************************************************************
* Class:  CSC-415-01 - Spring 2023
* Names: Yan Peng, Zhenyu Lin, Zaijing Liu, Zicheng Tan
* Student IDs: 921759056, 920225329, 921394952, 920261872
* GitHub Name: yuqiao1205
* Group Name: Chinese
* Project: Basic File System
*
* File: fsVol.h
*
* Description: The header file contains declarations of functions,
*              definitions of data types and
*              macros that are used for VCB.
*
**************************************************************/
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fsLow.h"

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif
#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif

#define MY_MAGIC_NUMBER	0x79705f7a6c5f7a6c
#define MY_MAGIC_NUMBER2 0x5f7a745f66733031

#ifndef _FS_VOL
#define _FS_VOL

typedef struct VolumeControlBlock {

    long int magicNumber;
    long int magicNumber2;

    uint64_t numberOfBlocks;
    uint64_t blockSize;

    uint64_t freeSpaceMapPosition;
    uint64_t rootDirectoryPosition;

    // internal use only
    uint64_t freeSpaceBlocks;
} VolumeControlBlock;

extern uint64_t blockSize;

uint64_t read_volume_control_block(VolumeControlBlock * vcb);
int check_magic_number(VolumeControlBlock * vcb);
int init_volume_control_block(VolumeControlBlock * vcb, 
                              uint64_t numberOfBlocks, 
                              uint64_t blockSize);
uint64_t write_volume_control_block(VolumeControlBlock * vcb); 

#endif