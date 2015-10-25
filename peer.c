/*
 * peer.c
 *
 * Authors: Ed Bardsley <ebardsle+441@andrew.cmu.edu>,
 *          Dave Andersen
 * Class: 15-441 (Spring 2005)
 *
 * Skeleton for 15-441 Project 2.
 *
 */

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "spiffy.h"
#include "bt_parse.h"
#include "input_buffer.h"
#include "sha.h"
#include "chunk.h"
#include <assert.h>

#define MAX_LINE_LENGTH 255
#define CHUNK_HASH_SIZE 41
#define MAGIC_NUMBER 15441
#define VERSION_NUMBER 1
#define HEADER_LENGTH 16
#define WHOHAS 0
#define IHAVE 1
#define GET 2
#define DATA 3
#define ACK 4
#define DENIED 5
#define CHUNK_PER_PACKET 74

/* TODO: chunk should have a corresponding filename 
 * change to hashtable? see common.h */
struct has_chunk{
  int id;
  char hash[SHA1_HASH_SIZE];
  struct has_chunk* next;
};

int sock;
bt_config_t config;
struct has_chunk* has_chunks;

void peer_run(bt_config_t *config);

void print_packet(char* packet, unsigned short packet_length){
  unsigned short magic_number = *(unsigned short*)packet;
  unsigned char version_number = *(unsigned char*)(packet+2);
  unsigned char packet_type = *(unsigned char*)(packet+3);
  unsigned short header_length = *(unsigned short*)(packet+4);
  unsigned short in_packet_length = *(unsigned short*)(packet+6);
  unsigned int seq_number = *(unsigned int*)(packet+8);
  unsigned int ack_number = *(unsigned int*)(packet+12);
  printf("Printing packet....................................................................\n");
  printf("magic_number: %hu, version_number: %hhu, packet_type: %hhu\n", magic_number, version_number, packet_type);
  printf("header_length: %hu, packet_length: %hu\n", header_length, in_packet_length);
  printf("seq_number: %u, ack_number: %u\n", seq_number, ack_number);
  unsigned short i = 0;
  char ascii_buf[CHUNK_HASH_SIZE];
  for(i=20;i<packet_length;i=i+20){
    binary2hex((uint8_t *)(packet+i), SHA1_HASH_SIZE, ascii_buf);
    printf("hash: %s\n", ascii_buf);
  }
  printf("End printing packet....................................................................\n");
}

void print_chunks(struct has_chunk* chunks){
  struct has_chunk* p;
  p = chunks;
  unsigned int i = 0;
  char temp[CHUNK_HASH_SIZE];
  while(p!=NULL){
    binary2hex((uint8_t*)p->hash, SHA1_HASH_SIZE, temp);
    printf("%d: %s\n", p->id, temp);
    p = p->next;
    i++;
  }
  printf("Total chunks: %d\n", i);
}

void fill_header(char* packet, unsigned char packet_type, unsigned short packet_length, unsigned int seq_number, unsigned int ack_number){
  unsigned short magic_number = MAGIC_NUMBER;
  unsigned char version_number = VERSION_NUMBER;
  short header_length = HEADER_LENGTH;
  *(unsigned short*)packet = htons(magic_number);
  *(unsigned char*)(packet+2) = version_number;
  *(unsigned char*)(packet+3) = packet_type;
  *(unsigned short*)(packet+4) = htons(header_length);
  *(unsigned short*)(packet+6) = htons(packet_length);
  *(unsigned int*)(packet+8) = htonl(seq_number);
  *(unsigned int*)(packet+12) = htonl(ack_number);
}

int find_chunk(char* hash, struct has_chunk* chunks){
  struct has_chunk* p_chunk = chunks;
  // char ascii_buf1[CHUNK_HASH_SIZE];
  // char ascii_buf2[CHUNK_HASH_SIZE];
  while(p_chunk!=NULL){
    // binary2hex((uint8_t *)hash, SHA1_HASH_SIZE, ascii_buf1);
    // binary2hex((uint8_t *)p_chunk->hash, SHA1_HASH_SIZE, ascii_buf2);
    // printf("hash: %s,%s\n", ascii_buf1, ascii_buf2);
    if (memcmp(hash, p_chunk->hash, SHA1_HASH_SIZE) == 0) {
      return 1;
    }
    p_chunk = p_chunk->next;
  }
  return -1;
}

void send_one_whohas_ihave_packet(struct sockaddr* dst_addr, unsigned char packet_type, struct has_chunk* chunks, unsigned char chunk_count, unsigned char offset){
  unsigned int packet_length = 20; // initial to header length
  unsigned char i, j;
  struct has_chunk* p = chunks;
  packet_length += chunk_count * SHA1_HASH_SIZE; // add chunk length
  char* packet = (char*)malloc(packet_length);
  fill_header(packet, packet_type, packet_length, 0, 0);
  *(unsigned char*)(packet+16) = chunk_count;
  i = 0;
  while(p!= NULL){
    if(i==offset){
      break;
    }
    i++;
    p = p->next;
  }
  j = 0;
  while(p!=NULL){
    memcpy(packet+20+SHA1_HASH_SIZE*j, p->hash, SHA1_HASH_SIZE);
    p = p->next;
    j++;
    if(j==chunk_count){
      break;
    }
  }
  assert(j==chunk_count);
  spiffy_sendto(sock, packet, packet_length, 0, dst_addr, sizeof(*dst_addr));
  // sendto(sock, packet, packet_length, 0, dst_addr, sizeof(*dst_addr));
  printf("sent one packet to %s:%d\n", inet_ntoa(((struct sockaddr_in*)dst_addr)->sin_addr),ntohs(((struct sockaddr_in*)dst_addr)->sin_port));
}

void send_whohas_ihave_packet(struct sockaddr* dst_addr, unsigned char packet_type, struct has_chunk* chunks){
  unsigned int chunk_count = 0;
  unsigned int packet_count = 0;
  unsigned int j;
  struct has_chunk* p = chunks;
  while(p!= NULL){
    chunk_count++;
    p = p->next;
  }
  packet_count = chunk_count/CHUNK_PER_PACKET;
  printf("in send_whohas_ihave_packet, chunk_count: %d, packet_count: %d\n", chunk_count, packet_count);
  if(packet_count==0){
    send_one_whohas_ihave_packet(dst_addr, packet_type, chunks, chunk_count, 0);
  }
  else{
    for(j=0;j<packet_count;j++){
      if(j!=packet_count-1){
        send_one_whohas_ihave_packet(dst_addr, packet_type, chunks, CHUNK_PER_PACKET, j*CHUNK_PER_PACKET);
      }
      else{
        if(packet_count*CHUNK_PER_PACKET<chunk_count){
          send_one_whohas_ihave_packet(dst_addr, packet_type, chunks, CHUNK_PER_PACKET, j*CHUNK_PER_PACKET);
          send_one_whohas_ihave_packet(dst_addr, packet_type, chunks, chunk_count-packet_count*CHUNK_PER_PACKET , (j+1)*CHUNK_PER_PACKET);
        }
        else if (packet_count*CHUNK_PER_PACKET==chunk_count){
        	send_one_whohas_ihave_packet(dst_addr, packet_type, chunks, CHUNK_PER_PACKET, j*CHUNK_PER_PACKET);
        }
      }
    }
  }
}

int main(int argc, char **argv) {

  bt_init(&config, argc, argv);

  DPRINTF(DEBUG_INIT, "peer.c main beginning\n");

#ifdef TESTING
  config.identity = 1; // your group number here
  strcpy(config.chunk_file, "chunkfile");
  strcpy(config.has_chunk_file, "haschunks");
#endif

  bt_parse_command_line(&config);

#ifdef DEBUG
  if (debug & DEBUG_INIT) {
    bt_dump_config(&config);
  }
#endif
  
  peer_run(&config);
  return 0;
}

void process_inbound_udp(int sock) {
  #define BUFLEN 1500
  struct sockaddr_in from;
  socklen_t fromlen;
  char buf[BUFLEN];
  unsigned char chunk_count;
  unsigned char i;
  int recv_bytes;
  
  fromlen = sizeof(from);
  recv_bytes = spiffy_recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *) &from, &fromlen);
  
  if(recv_bytes <= 0) {
  	/* conn closed or error occurred, close the socket */
  }
  
  unsigned short magic_number = *(unsigned short*)buf;
  unsigned char version_number = *(unsigned char*)(buf+2);
  unsigned char packet_type = *(unsigned char*)(buf+3);
  unsigned short header_length = *(unsigned short*)(buf+4);
  unsigned short in_packet_length = *(unsigned short*)(buf+6);
  unsigned int seq_number = *(unsigned int*)(buf+8);
  unsigned int ack_number = *(unsigned int*)(buf+12);
  
  char * filename;
  char * data_buf = buf + header_length;
  unsigned data_length = in_packet_length - header_length;
  unsigned last_continuous_seq;
  unsigned peer_id = bt_peer_id(sock);
  char chunk_hash[SHA1_HASH_SIZE];
  int hash_correct;
  int ack_count;
  
  /* check the format of the packet, i.e. magic number 15441 and version 1 */
  if(magic_number != 15441 || version_number != 1)
  	return;
  
  /* should handle DATA lost and GET lost. If WHOHAS is guaranteed to get 
   * an IHAVE reponse, handle it as well. Use a queue with timestamp to 
   * calculate when timeout */
  switch(packet_type){
    case WHOHAS:
      // reply if it has any of the packets that the WHOHAS packet inquires
      chunk_count = *(unsigned char*)(buf+16);
      i=0;
      struct has_chunk* chunks = NULL;
      struct has_chunk* p_chunk = NULL;
      printf("WHOHAS packet received, chunk_count: %d, %d\n", chunk_count, i);
      while(i<chunk_count){
        if(find_chunk(buf+20+i*SHA1_HASH_SIZE, has_chunks)==1){
          p_chunk = (struct has_chunk*)malloc(sizeof(struct has_chunk));
          p_chunk->id = 0;
          memcpy(p_chunk->hash, buf+20+SHA1_HASH_SIZE*i, SHA1_HASH_SIZE);
          p_chunk->next = chunks;
          chunks = p_chunk;
        }
        i++;
      }
      if(chunks!=NULL){
        printf("Send back IHAVE packets, %d\n", i);
        send_whohas_ihave_packet((struct sockaddr *)&from, IHAVE, chunks);
      }
      break;
    case IHAVE:
    	/* TODO: select a packet to download */
    	chunk_hash = pick_a_chunk(packet);
    	/* add a transmission stream, i.e. associate the stream with peer */
    	if(start_download(peer_id, chunk_hash))    	
    		Send_GET(sock, chunk_hash);/* TODO: send GET request */
      break;
    case GET:
    	/* if find chunk != NULL*/
    	
    	/* TODO: init a transmission stream and respond data 
    	 * check if upload queue is full, if not, start uploading */
    	start_upload(sock, chunk_hash);
    	break;
    case DATA:
    	/* get the corresponding chunk of the peer */
    	chunk_hash = get_chunk_hash(peer_id);
    	if(chunk_hash != NULL) {
		  	/* get filename from hash table */
		  	filename = find_chunk(chunk_hash).filename;
		  	/* check if the hash is correct, TODO: how to compute hash? */
		  	hash_correct = !strcmp(compute_hash(data_buf, data_length), chunk_hash);
		  	/* get a new data packet, write the data to storage
		  	 * seq_number starts with 1 */
		  	if(hash_correct)
		  		write_file(filename, data_buf, data_length, seq_number - 1);
    	}
    	if(hash_correct) {
		  	/* keep track of the packet */
		  	last_continuous_seq = keep_track(peer_id, seq_number, data_length);
		  	/* TODO: send an ACK */
		  	Send_ACK(sock, last_continuous_seq);
    	}
    	break;
    case ACK:
    	/* move pointer */
    	ack_count = receive_ack(peer_id, ack_number);
    	window_control(peer_id, ack_count);
    	break;
    case DENIED:
    	abort_download(peer_id);
    	/* remove the transmission stream */
    	break;
    default:
      printf("Unexpected packet type!\n");
      break;
  }
}

// parse has chunk file
void parse_has_get_chunk_file(char* has_get_chunk_file, struct has_chunk** has_get_chunks){
  FILE *f;
  int chunk_id;
  char hash[CHUNK_HASH_SIZE];
  char binary_hash[SHA1_HASH_SIZE];
  struct has_chunk* p_has_chunk;
  char line[MAX_LINE_LENGTH];
  assert(has_get_chunk_file != NULL);
  f = fopen(has_get_chunk_file, "r");
  assert(f != NULL);
  while (fgets(line, MAX_LINE_LENGTH, f) != NULL) {
    if (line[0] == '#'){
      continue;
    }
    assert(sscanf(line, "%d %s", &chunk_id, hash) != 0);
    p_has_chunk = (struct has_chunk*)malloc(sizeof(struct has_chunk));
    assert(p_has_chunk != NULL);
    p_has_chunk->id = chunk_id;
    hex2binary(hash, CHUNK_HASH_SIZE-1, (uint8_t*)binary_hash);
    char tempchar1[CHUNK_HASH_SIZE];
    char tempchar2[CHUNK_HASH_SIZE];
    // binary2hex(hash, CHUNK_HASH_SIZE-1, (uint8_t*)binary_hash);
    binary2hex((uint8_t *)binary_hash, SHA1_HASH_SIZE, tempchar1);
    strncpy(p_has_chunk->hash, binary_hash, sizeof(binary_hash));
    binary2hex((uint8_t *)p_has_chunk->hash, SHA1_HASH_SIZE, tempchar2);
    p_has_chunk->next = *has_get_chunks;
    *has_get_chunks = p_has_chunk;
  }
  fclose(f);
}

void process_get(char *chunkfile, char *outputfile) {
  struct has_chunk* get_chunks;
  get_chunks = NULL;
  parse_has_get_chunk_file(chunkfile, &get_chunks);
  bt_peer_t* p_peer = config.peers;
  while(p_peer!=NULL){
    if(p_peer->id != config.identity){
      send_whohas_ihave_packet((struct sockaddr*)&p_peer->addr, WHOHAS, get_chunks);
    }
    p_peer = p_peer->next;
  }
}



void handle_user_input(char *line, void *cbdata) {
  char chunkf[128], outf[128];

  bzero(chunkf, sizeof(chunkf));
  bzero(outf, sizeof(outf));

  if (sscanf(line, "GET %120s %120s", chunkf, outf)) {
    if (strlen(outf) > 0) {
      process_get(chunkf, outf);
    }
  }
}


void peer_run(bt_config_t *config) {
  struct sockaddr_in myaddr;
  fd_set readfds;
  struct user_iobuf *userbuf;
  has_chunks = NULL;
  if ((userbuf = create_userbuf()) == NULL) {
    perror("peer_run could not allocate userbuf");
    exit(-1);
  }
  
  if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1) {
    perror("peer_run could not create socket");
    exit(-1);
  }
  
  bzero(&myaddr, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(config->myport);
  
  if (bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
    perror("peer_run could not bind socket");
    exit(-1);
  }
  
  /* init tracker and controller */
  init_tracker(config->max_conn);
  init_controller(config->max_conn);
  
  spiffy_init(config->identity, (struct sockaddr *)&myaddr, sizeof(myaddr));
  parse_has_get_chunk_file(config->has_chunk_file, &has_chunks);
  
// #ifdef DEBUG
//   if (debug & DEBUG_ERRS) {
//     struct has_chunk* p = has_chunks;
//     while(p!=NULL){
//       printf("%d %s\n", p->id, p->hash);
//       p = p->next;
//     }
//   }
// #endif
  
  while (1) {
    int nfds;
    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(sock, &readfds);
    
    nfds = select(sock+1, &readfds, NULL, NULL, NULL);
    
    if (nfds > 0) {
      if (FD_ISSET(sock, &readfds)) {
        process_inbound_udp(sock);
      }
      
      if (FD_ISSET(STDIN_FILENO, &readfds)) {
        process_user_input(STDIN_FILENO, userbuf, handle_user_input, "Currently unused");
      }
    }
  }
}
