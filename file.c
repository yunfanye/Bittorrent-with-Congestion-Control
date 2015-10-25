#include "file.h"
#include <unistd.h>
#include <sys/types.h>

/* open or create a file */
int open_file(char * filename) {
	int fd = open(filename, O_RDWR|O_CREAT, 0640);
	if(fd < 0)
		printf("Open file error!\n");
	return fd;
}

/* create a file with fixed size, i.e. allocate a free space on storage */
int create_file(char * filename, int size) {
	int ret;
	int fd = open_file(filename);
	lseek(fd, size - 1, SEEK_SET); /* assume good input */
	ret = write(fd, "", 1);
	close_file(fd);
	return ret;
}

/* write buf to file, starting from offset and writing continous
 * length bytes */
int write_file(char * filename, char * buf, int length, int offset) {
	int ret;
	int fd = open_file(filename);
	lseek(fd, offset, SEEK_SET); /* assume good input */
	ret = write(fd, buf, length);
	close_file(fd);
	return ret;
}

/* close the file */
void close_file(int fd) {
	int rc;
	if ((rc = close(fd)) < 0)
		printf("close file error\n");
}
