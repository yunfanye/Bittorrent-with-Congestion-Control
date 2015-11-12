#include "file.h"
#include "window_log.h"
#include <unistd.h>
#include <string.h>

#define BUF_SIZE 255

static char * log_file_name =	"problem2-peer.txt";

static int log_fd;

int open_log() {
	log_fd = open_file(log_file_name);
	return log_fd;
}

int log_window(int id, int size, unsigned long time) {
	int writeret;
	char buf[BUF_SIZE];
	sprintf(buf, "f%d %d %lu\n", id, size, time);
	writeret = write(log_fd, buf, strlen(buf));
	return writeret;
}
