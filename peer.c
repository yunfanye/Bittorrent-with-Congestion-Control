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
  bt_dump_config(&config);
  peer_run(&config);
  return 0;
}

void process_inbound_udp(int sock) {
  #define BUFLEN 1500
  struct sockaddr_in from;
  socklen_t fromlen;
  char buf[BUFLEN];
  int recv_bytes;
  fromlen = sizeof(from);
  
  recv_bytes = spiffy_recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *) &from, &fromlen);
  if(recv_bytes <= 0) {
  	/* conn closed or error occurred, close the socket */
  }
  
  unsigned short magic_number = ntohs(*(unsigned short*)buf);
  unsigned char version_number = *(unsigned char*)(buf+2);
  unsigned char packet_type = *(unsigned char*)(buf+3);
  unsigned short header_length = ntohs(*(unsigned short*)(buf+4));
  unsigned short in_packet_length = ntohs(*(unsigned short*)(buf+6));
  unsigned int seq_number =ntohl(*(unsigned int*)(buf+8));
  unsigned int ack_number = ntohl(*(unsigned int*)(buf+12));
  struct packet* incoming_packet = (struct packet*)buf;
  // validate packet
  if(validate_packet(magic_number, version_number, packet_type)<0){
    printf("Invalid packet\n");
    return;
  }

  // char * filename;
  // char * data_buf = buf + header_length;
  unsigned data_length = in_packet_length - header_length;
  unsigned last_continuous_seq;
  short peer_id = bt_peer_id(from);
  uint8_t* chunk_hash = NULL;
  int ack_count;
  struct packet* packet;
  struct Chunk* p_chunk;
  int chunk_id;
  /* should handle DATA lost and GET lost. If WHOHAS is guaranteed to get 
   * an IHAVE reponse, handle it as well. Use a queue with timestamp to 
   * calculate when timeout */
  switch(packet_type){
    case WHOHAS:
    	printf("receive WHOHAS\n");
      // reply if it has any of the packets that the WHOHAS packet inquires
      // print_packet(incoming_packet);
      packet = make_packet(IHAVE, NULL, NULL, 0, 0, 0, incoming_packet, NULL, NULL);
      if(packet!=NULL){
        print_packet(packet);
        send_packet(*packet, sock, (struct sockaddr*)&from);
        free_packet(packet);
      }
      break;
    case IHAVE:
    	printf("\nGET IHAVE: %d\n", peer_id);
    	/* TODO conversion */
    	print_incoming_packet(incoming_packet);
      update_connections(peer_id, incoming_packet);
      print_connections();
      if(current_request!=NULL){
        // TODO: may need to update timestamp for the new current_request
        // pick a chunk and mark the chunk as RECEIVING
        chunk_hash = pick_a_chunk(incoming_packet, &p_chunk);
        printf("picked a chunk,  %d", p_chunk->id);
        print_hash(chunk_hash);
        if(chunk_hash!=NULL){
          /* add a transmission stream, i.e. associate the stream with peer */
          mark_peer_state(peer_id, WORKING);
          printf("start downloading\n");
          if(start_download(peer_id, chunk_hash)){
          	printf("sent GET packet\n");
            packet = make_packet(GET, p_chunk, NULL, 0, 0, 0, NULL, NULL, NULL);
            /* Send GET */
            send_packet(*packet, sock, (struct sockaddr*)&from);
            /* TODO: fix memory leakage */
            free_packet(packet);
          }
          printf("end downloading\n");
        }
        else{
          mark_peer_state(peer_id, NOT_WORKING);
        }
      }
      break;
    case GET:
    	printf("\nGot GET Packet\n"); 	
      chunk_hash = (uint8_t*)((char*)incoming_packet + header_length);
      /* if find chunk != NULL*/
  		if(find_chunk(chunk_hash)==1){
    	/* init a transmission stream and respond data 
    	 * check if upload queue is full, if not, start uploading */
        printf("peer_id: %d, GET packet here, chunk_id: %d\n", peer_id, get_chunk_id(chunk_hash, has_chunk_table));
		    if(start_upload(peer_id, get_chunk_id(chunk_hash, has_chunk_table))){
          printf("start_upload success\n"); 
		  	 	init_cwnd(peer_id);/* init cwnd */
        }
    	}
    	break;
    case DATA:
    	/* get the corresponding chunk of the peer */
    	chunk_hash = get_chunk_hash(peer_id);
      chunk_id = get_chunk_id(chunk_hash, current_request);
      if(chunk_id < 0){
        return;
      }
      /* keep track of the packet */
      last_continuous_seq = track_data_packet(peer_id, seq_number, data_length);
      // ignore historical packets
      printf("GOT data pack: %d, last: %d\n", seq_number, last_continuous_seq);
      if(seq_number<last_continuous_seq){
        printf("Receive historical data packet, ack with last_continuous_seq\n");
        packet = make_packet(ACK, NULL, NULL, 0, 0, last_continuous_seq, NULL, NULL, NULL);
        send_packet(*packet, sock, (struct sockaddr*)&from);
        free_packet(packet);
        break;
      } 
      // later packet arrived first, send duplicate ACK
      else if(seq_number > last_continuous_seq){
      		printf("DOWNLOAD TIMOUT, SEND DUP ACKS!\n");
          packet = make_packet(ACK, NULL, NULL, 0, 0, last_continuous_seq, NULL, NULL, NULL);
          send_packet(*packet, sock, (struct sockaddr*)&from);
          free_packet(packet);
          /* discard the packet as it is very difficult to save the reordered
           * packet due to the stupid protocol */
          break;
      }
      // expected packet
      else{
          packet = make_packet(ACK, NULL, NULL, 0, 0, last_continuous_seq, NULL, NULL, NULL);
          send_packet(*packet, sock, (struct sockaddr*)&from);
          free_packet(packet);
          // save data to chunk until chunk is filled
          // each chunk has 512*1024 bytes, each packet has 1500-16 max bytes data
          save_data_packet(incoming_packet, chunk_id);
          // save the whole chunk if finished
          // verify chunk is done within the function
          if(save_chunk(chunk_id) > 0) {
            // Download next chunk if exists
            printf("download completed!\n");
            abort_download(peer_id);
            chunk_hash = pick_a_new_chunk(peer_id, &p_chunk);
            printf("picked a chunk,  %d", p_chunk->id);
            print_hash(chunk_hash);
            if(chunk_hash!=NULL){
              mark_peer_state(peer_id, WORKING);
              /* add a transmission stream, i.e. associate the stream with peer */
              if(start_download(peer_id, chunk_hash)){
                packet = make_packet(GET, p_chunk, NULL, 0, 0, 0, NULL, NULL, NULL);
                send_packet(*packet, sock, (struct sockaddr*)&from);
                free_packet(packet);
              }
            }
            else{
              mark_peer_state(peer_id, NOT_WORKING);
            }
          }
      }
    	break;
    case ACK:
    	/* TODO: move pointer */
      // print_incoming_packet(incoming_packet);
    	printf("ack %d\n", ack_number);
    	ack_count = receive_ack(peer_id, ack_number);
    	window_control(peer_id, ack_count);
      if(ack_number==MAX_PACKET_PER_CHUNK){
        printf("all ACK received, finished UPLOAD\n");
        abort_upload(peer_id);
      }
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
  //struct has_chunk* get_chunks;
  //get_chunks = NULL;
  current_request = parse_has_get_chunk_file(chunkfile, outputfile);
  print_request(current_request);
  whohas_flooding(current_request);
  printf("whohas flooding ends\n");
}

void handle_user_input(char *line, void *cbdata) {
  char chunkf[128], outf[128];

  bzero(chunkf, sizeof(chunkf));
  bzero(outf, sizeof(outf));

  if (sscanf(line, "GET %120s %120s", chunkf, outf)) {
    if (strlen(outf) > 0) {
    	create_file(outf, BT_CHUNK_SIZE);
      process_get(chunkf, outf);
    }
  }
}


void peer_run(bt_config_t *config) {
  struct sockaddr_in myaddr;
  fd_set readfds;
  struct user_iobuf *userbuf;
  struct timeval timeout;
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

	open_log();
  /* init tracker and controller */
  init_tracker(config->max_conn);
  init_controller(config->max_conn);
  current_request = NULL;
  connections = NULL;
  has_chunk_table = NULL;
  spiffy_init(config->identity, (struct sockaddr *)&myaddr, sizeof(myaddr));
  has_chunk_table = parse_has_get_chunk_file(config->has_chunk_file, NULL);
  print_request(has_chunk_table);

  char read_buffer[MAX_LINE_LENGTH];
  FILE* master_chunk_file = fopen(config->chunk_file, "r");
  fgets(read_buffer, MAX_LINE_LENGTH, master_chunk_file);
  fclose(master_chunk_file);
  /* parse to get the path */
  memset(master_data_file_name, 0, FILE_NAME_SIZE);
  sscanf(read_buffer, "File:%s", master_data_file_name);
  printf("master_data_file_name: %s\n", master_data_file_name);

  while (1) {
    int nfds;
    timeout.tv_sec = 0;
  	timeout.tv_usec = 500000; /* 50 ms */
    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(sock, &readfds);
    nfds = select(sock+1, &readfds, NULL, NULL, &timeout); 
    if (nfds > 0) {
      if (FD_ISSET(sock, &readfds)) {
        process_inbound_udp(sock);
      }
      
      if (FD_ISSET(STDIN_FILENO, &readfds)) {
        process_user_input(STDIN_FILENO, userbuf, handle_user_input, "Currently unused");
      }
    }
    send_data_packets();
		clean_upload_timeout();
		download_peer_crash_wrapper();
    // Need to do whohas_flooding periodically
    // if(get_time_diff(&last_flood_whohas_time) > WHOHAS_FLOOD_INTERVAL_MS){
    //     whohas_flooding(current_request);
    //     gettimeofday(&last_flood_whohas_time, NULL);
    // }
    if(all_chunk_finished(current_request)){
      printf("all chunk finished, GET request DONE\n");
      free(current_request->filename);      
      free(current_request);
      current_request = NULL;
    }
  }
}
