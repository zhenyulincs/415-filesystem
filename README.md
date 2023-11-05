# SFSU - CSC415 Group Term Assignment - File System
## Project Overview
Our file system can manage the files and directories based on a hierarchical filesystem, typified by the file system in Linux. This file system uses a contiguous allocation scheme to allocate disk space to files, each file occupies a contiguous series of blocks on disk. We used a free space bitmap that tracks what blocks on the disk have been allocated. With this system, storage devices must be partitioned and formatted before the first use. We initialize a volume control block, free space map and root directory. Beyond that, the file system is typically composed of two main components: a directory structure and a set of data blocks containing files and directories. The directory structure is organized as a tree with the root directory serving as the anchor. Each directory consists of a table of directory entries, which provides information on files and directories and which data blocks store the actual file contents. Lastly, our file system provides an interactive interface for users to use commands including (ls, cp, mv, md, rm, touch, cat, cp2l, cp2fs, cd, pwd, history, help) to manage the file system. The file system uses a main driver fsshell.c to dispatch commands entered into the shell. These in turn call on functions in mfs.c and b_io.c which utilize lower-level implementations in fsVol (volume), fsFree (free space map), and fsDir (directory and path management).


Project Members
| Name        | Github Username |
|-------------|-----------------|
| Yan Peng    | yuqiao1205      |
| Zicheng Tan | Damon0427       |
| Zaijing Liu | Chlora06        |
| Zhenyu Lin  | zhenyulincs     |
