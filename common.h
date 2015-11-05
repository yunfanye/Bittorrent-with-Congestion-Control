#include "sha.h"

#ifndef _COMMON_H_
#define _COMMON_H_

#define FILE_NAME_SIZE	255

/* each chunk */
struct Chunk {
	char hash[SHA1_HASH_SIZE];
	char filename[FILE_NAME_SIZE]; /* output filename */
	int state;	/* 0 owned, 1 receiving */
};

/* owned sha1 hash */
struct Chunk ** chunkTable;

#endif
