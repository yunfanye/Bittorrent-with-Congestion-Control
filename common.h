#ifndef _COMMON_H_
#define _COMMON_H_

#include "sha.h"

#define FILE_NAME_SIZE	255

#define OWNED 0
#define RECEIVING 1
#define NOT_STARTED 2

#define CHUNK_DATA_SIZE 


/* each chunk */
struct Chunk {
	int id;
	char hash[SHA1_HASH_SIZE];
	int state;	/* 0 owned, 1 receiving */
	char* data;
	int received_seq_number;
	int received_byte_number;
};

struct Request{
	char filename[FILE_NAME_SIZE];
    int chunk_number;
    struct Chunk* chunks;
};



/* owned sha1 hash */
struct Chunk *chunkTable;

#endif
