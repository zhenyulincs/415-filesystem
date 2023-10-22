/**************************************************************
* Class:  CSC-415-01 - Spring 2023
* Names: Yan Peng, Zhenyu Lin, Zaijing Liu, Zicheng Tan
* Student IDs: 921759056, 920225329, 921394952, 920261872
* GitHub Name: yuqiao1205
* Group Name: Chinese
* Project: Basic File System
*
* File: fsVol.c
*
* Description: This file for tracking VCB and helper functions
*
**************************************************************/
#include "fsVol.h"
#include "fsLow.h"
uint64_t blockSize;

uint64_t read_volume_control_block(VolumeControlBlock *vcb) {
    uint64_t result = LBAread(vcb, 1, 0);
    blockSize = vcb->blockSize;
    return result;
}

int check_magic_number(VolumeControlBlock *vcb) {
    printf("MagicNumberCheck: m1=%ld(%ld),m2=%ld(%ld)\n",
           vcb->magicNumber, MY_MAGIC_NUMBER,
           vcb->magicNumber2, MY_MAGIC_NUMBER2);
    return (vcb->magicNumber == MY_MAGIC_NUMBER) && (vcb->magicNumber2 == MY_MAGIC_NUMBER2);
}

int init_volume_control_block(VolumeControlBlock *vcb,
                              uint64_t numberOfBlocks,
                              uint64_t blockSizeIn) {

    vcb->magicNumber = MY_MAGIC_NUMBER;
    vcb->magicNumber2 = MY_MAGIC_NUMBER2;
    vcb->numberOfBlocks = numberOfBlocks;
    vcb->blockSize = blockSizeIn;
    blockSize = blockSizeIn;
    return 0;
}

uint64_t write_volume_control_block(VolumeControlBlock *vcb) {
    printf("write_volume_control_block\n");
    return LBAwrite(vcb, 1, 0);
}

