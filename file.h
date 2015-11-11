#ifndef _FILE_H_
#define _FILE_H_

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

/* open or create a file */
int open_file(char *pathname);

/* create a file with fixed size */
int create_file(char * filename, int size);

/* read from file, write to buf, starting from offset and writing continous
 * length bytes */
int read_file(char * filename, char * buf, int length, int offset)

/* write buf to file, starting from offset and writing continous
 * length bytes */
int write_file(char * filename, char * buf, int length, int offset);

/* close the file */
void close_file(int fd);

#endif
