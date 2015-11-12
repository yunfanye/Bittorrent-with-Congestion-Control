#include "file.h"
#include "window_log.h"
#include <unistd.h>
#include <string.h>
#include "util.h"

#define BUF_SIZE 255

static char * log_file_name =	"problem2-peer.txt";

static int log_fd;

static unsigned long base_time;

int open_log() {
	base_time = milli_time();
	log_fd = open(log_file_name, O_RDWR|O_CREAT|O_TRUNC, 0640);
	return log_fd;
}

int log_window(int id, int size, unsigned long time) {
	int writeret;
	char buf[BUF_SIZE];
	sprintf(buf, "f%d %d %lu\n", id, size, (time - base_time));
	writeret = write(log_fd, buf, strlen(buf));
	return writeret;
}
