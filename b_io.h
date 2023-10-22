/**************************************************************
* Class:  CSC-415-01 - Spring 2023
* Names: Yan Peng, Zhenyu Lin, Zaijing Liu, Zicheng Tan
* Student IDs: 921759056, 920225329, 921394952, 920261872
* GitHub Name: yuqiao1205
* Group Name: Chinese
* Project: Basic File System
*
* File: b_io.h
*
* Description: Interface of basic I/O functions
*
**************************************************************/

#ifndef _B_IO_H
#define _B_IO_H
#include <fcntl.h>

typedef int b_io_fd;

b_io_fd b_open (char * filename, int flags);
int b_read (b_io_fd fd, char * buffer, int count);
int b_write (b_io_fd fd, char * buffer, int count);
int b_seek (b_io_fd fd, off_t offset, int whence);
int b_close (b_io_fd fd);

#endif

