#ifndef _PEER_H_
#define _PEER_H_

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "debug.h"
#include "spiffy.h"
#include "bt_parse.h"
#include "input_buffer.h"
#include "sha.h"
#include "chunk.h"
#include "network.h"
#include "keep_track.h"
#include "common.h"
#include "util.h"
#include "window_log.h"



/* TODO: chunk should have a corresponding filename 
 * change to hashtable? see common.h */

int sock;
bt_config_t config;
struct Request* has_chunk_table; // used to record has_chunk number and chunks
struct Request* total_chunk_table;
struct Request* current_request;
struct connection* connections;
char master_data_file_name[FILE_NAME_SIZE];

void peer_run(bt_config_t *config);
int main(int argc, char **argv);
void process_inbound_udp(int sock);
void process_get(char *chunkfile, char *outputfile);
void handle_user_input(char *line, void *cbdata);
void peer_run(bt_config_t *config);
#endif
