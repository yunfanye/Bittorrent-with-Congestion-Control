#ifndef _FILE_H_
#define _FILE_H_

/* open or create a file */
int open_file(const char *pathname);

/* create a file with fixed size */
int create_file(char * filename, int size);

/* write buf to file, starting from offset and writing continous
 * length bytes */
int write_file(char * filename, char * buf, int length, int offset);

/* close the file */
int close_file(int fd);

#endif
