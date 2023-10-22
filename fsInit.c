/**************************************************************
* Names: Yan Peng, Zhenyu Lin, Zaijing Liu, Zicheng Tan
* Student IDs: 921759056, 920225329, 921394952, 920261872
* GitHub Name: yuqiao1205
* Group Name: Chinese
* Project: Basic File System
*
* File: fsInit.c
*
* Description: Main driver for file system assignment.
*
* This file is where you will start and initialize your system
*
**************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "fsLow.h"
#include "mfs.h"

#include "fsVol.h"
#include "fsFree.h"
#include "fsDir.h"

int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	printf ("Initializing File System with %ld blocks with a block size of %ld\n",
            numberOfBlocks, blockSize);

	VolumeControlBlock * vcb = malloc(blockSize);
	printf("size of VCB: %ld\n", sizeof(VolumeControlBlock));
	read_volume_control_block(vcb);

	// ** read volume control block, free space map, and root directory

	if (check_magic_number(vcb)) {
		// already initialized
		printf("magic numbers found, already initialized\n");

		// read free space map and root directory
		read_free_space_map(vcb->freeSpaceMapPosition, vcb->freeSpaceBlocks);
		read_root_directory(vcb->rootDirectoryPosition);

		return 0;
	}

	printf("no magic numbers found, initializing VCB\n");
    // zero whole block including size of VolumeControlBlock (which may be smaller)
    memset(vcb, 0, blockSize);
	init_volume_control_block(vcb, numberOfBlocks, blockSize);

	init_free_space_map(vcb);

	vcb->rootDirectoryPosition = init_root_directory(blockSize);

	write_volume_control_block(vcb);

	return 0;
	}

void exitFileSystem ()
	{
	printf ("System exiting\n");
	}