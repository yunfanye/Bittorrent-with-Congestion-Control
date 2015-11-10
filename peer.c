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

#include "peer.h"

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
      struct packet* packet = make_packet(IHAVE, NULL, NULL, 0, 0, 0, packet_in, NULL, NULL);
      if(packet!=NULL){
        send_packet(packet, sock, (struct sockaddr*)&from);
        free(packet);
      }
      break;
    case IHAVE:
    	/* TODO: select a packet to download */
    	chunk_hash = pick_a_chunk(packet);
    	/* add a transmission stream, i.e. associate the stream with peer */
    	if(start_download(peer_id, chunk_hash)){
        struct packet* packet = make_packet(GET, p_chunk, NULL, 0, 0, 0, NULL, NULL, NULL);
        send_packet(packet, sock, (struct sockaddr*)&from);
        free(packet);
      }
      break;
    case GET:
    	/* if find chunk != NULL*/
  		if(find_chunk(chunk_hash)) {
    	/* init a transmission stream and respond data 
    	 * check if upload queue is full, if not, start uploading */
		  	if(start_upload(sock, chunk_hash))    	
		  		init_cwnd(peer_id);/* init cwnd */
    	}
    	break;
    case DATA:
    	/* get the corresponding chunk of the peer */
    	chunk_hash = get_chunk_hash(peer_id);
    	if(chunk_hash != NULL) {
		  	/* get filename from hash table */
		  	filename = find_chunk(chunk_hash).filename;
		  	/* get a new data packet, write the data to storage
		  	 * seq_number starts with 1 */
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
    	/* TODO: move pointer */
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


void process_get(char *chunkfile, char *outputfile){
  struct has_chunk* get_chunks;
  get_chunks = NULL;
  struct Request* new_get_request = parse_has_get_chunk_file(chunkfile, outputfile);
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
  chunkTable = NULL;
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
  has_chunk_table = parse_has_get_chunk_file(config->has_chunk_file, NULL);
  
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
