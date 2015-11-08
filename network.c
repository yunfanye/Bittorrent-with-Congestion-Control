#include "network.h"

// void print_packet(char* packet, unsigned short packet_length){
//   unsigned short magic_number = *(unsigned short*)packet;
//   unsigned char version_number = *(unsigned char*)(packet+2);
//   unsigned char packet_type = *(unsigned char*)(packet+3);
//   unsigned short header_length = *(unsigned short*)(packet+4);
//   unsigned short in_packet_length = *(unsigned short*)(packet+6);
//   unsigned int seq_number = *(unsigned int*)(packet+8);
//   unsigned int ack_number = *(unsigned int*)(packet+12);
//   printf("Printing packet....................................................................\n");
//   printf("magic_number: %hu, version_number: %hhu, packet_type: %hhu\n", magic_number, version_number, packet_type);
//   printf("header_length: %hu, packet_length: %hu\n", header_length, in_packet_length);
//   printf("seq_number: %u, ack_number: %u\n", seq_number, ack_number);
//   unsigned short i = 0;
//   char ascii_buf[CHUNK_HASH_SIZE];
//   for(i=20;i<packet_length;i=i+20){
//     binary2hex((uint8_t *)(packet+i), SHA1_HASH_SIZE, ascii_buf);
//     printf("hash: %s\n", ascii_buf);
//   }
//   printf("End printing packet....................................................................\n");
// }


void fill_header(char* packet_header, unsigned char packet_type, unsigned short packet_length, unsigned int seq_number, unsigned int ack_number){
  packet_header = (char*)malloc(16);
  unsigned short magic_number = MAGIC_NUMBER;
  unsigned char version_number = VERSION_NUMBER;
  short header_length = HEADER_LENGTH;
  *(unsigned short*)packet_header = htons(magic_number);
  *(unsigned char*)(packet_header+2) = version_number;
  *(unsigned char*)(packet_header+3) = packet_type;
  *(unsigned short*)(packet_header+4) = htons(header_length);
  *(unsigned short*)(packet_header+6) = htons(packet_length);
  *(unsigned int*)(packet_header+8) = htonl(seq_number);
  *(unsigned int*)(packet_header+12) = htonl(ack_number);
}

// Besides passing in type, also pass some necessary fields as parameters
// Other fields in the parameters can be arbitrary values
// To make a WHOHAS packet, pass in the request and packet_count
// To make a IHAVE packet, pass in a WHOHAS packet as packet_in
// To make a Get packet, pass in p_chunk
// 
struct packet* make_packet(unsigned char packet_type, struct Chunk* p_chunk, char* data, int data_size, int seq_number, int ack_number, struct packet* packet_in, int* packet_count, struct Request* request){
  unsigned char i = 0;
  int j = 0;
  int unfinished_chunk_count = 0;
  struct packet* packet = NULL;
  switch(packet_type){
    case WHOHAS:
      for(j=0;i<request->chunk_number;j++){
        // TODO: here may need to change add more state to chunks
        if(request->chunks[i].state == NOT_STARTED){
          unfinished_chunk_count++;
        }
      }
      if(unfinished_chunk_count%CHUNK_PER_PACKET==0){
        *packet_count = unfinished_chunk_count/CHUNK_PER_PACKET;
      }
      else{
        *packet_count = unfinished_chunk_count/CHUNK_PER_PACKET + 1; 
      }
      struct packet* packets = (struct packet*)malloc(sizeof(struct packet)* (*packet_count));
      if(unfinished_chunk_count==0){
        packets = NULL;
        free(packets);
      }
      int current_chunk_index = 0;
      for(j=0;j<(*packet_count);j++){
        unsigned char chunk_count = CHUNK_PER_PACKET;
        if(*packet_count <= j+1){
          chunk_count = (unsigned char)(unfinished_chunk_count - j*CHUNK_PER_PACKET);
        }
        fill_header(packets[j].header, WHOHAS, chunk_count*SHA1_HASH_SIZE+HEADER_LENGTH+CHUNK_NUMBER_AND_PADDING_SIZE, 0, 0);
        *(packets[j].payload) = chunk_count;
        int k=0;
        for(k=0;k<chunk_count;k++){
          for(; current_chunk_index<request->chunk_number;current_chunk_index++){
            if(request->chunks[current_chunk_index].state==NOT_STARTED){
              memcpy(packets[j].payload+CHUNK_NUMBER_AND_PADDING_SIZE+k*SHA1_HASH_SIZE,(request->chunks)[current_chunk_index].hash, SHA1_HASH_SIZE);
              current_chunk_index++;
              break;
            } 
          }
        }
      }
      return packets;
    case IHAVE:
      packet = (struct packet*)malloc(sizeof(struct packet));
      unsigned char chunk_count = *(packet_in->payload);
      unsigned char count_ihave = 0;
      for(i=0;i<chunk_count;i++){
        char* hash = (char*)(packet_in->payload+CHUNK_NUMBER_AND_PADDING_SIZE+i*SHA1_HASH_SIZE);
        if(find_chunk(hash)>0){
          memcpy(packet->payload+CHUNK_NUMBER_AND_PADDING_SIZE+count_ihave*SHA1_HASH_SIZE, hash, SHA1_HASH_SIZE);
          count_ihave++;
        }
      }
      if(count_ihave==0){
        free(packet);
        packet = NULL;
      }
      fill_header(packet->header, IHAVE, count_ihave*SHA1_HASH_SIZE+HEADER_LENGTH+CHUNK_NUMBER_AND_PADDING_SIZE, 0, 0);
      *(packet->payload) = count_ihave;
    case GET:
      packet = (struct packet*)malloc(sizeof(struct packet));
      fill_header(packet->header, GET, SHA1_HASH_SIZE+HEADER_LENGTH, 0, 0);
      memcpy(packet->payload, p_chunk->hash, SHA1_HASH_SIZE);
    case DATA:
      packet = (struct packet*)malloc(sizeof(struct packet));
      fill_header(packet->header, DATA, data_size+HEADER_LENGTH, seq_number, 0);
      memcpy(packet->payload, data, data_size);
    case ACK:
      packet = (struct packet*)malloc(sizeof(struct packet));
      fill_header(packet->header, ACK, HEADER_LENGTH, 0, ack_number);
  }
  return packet;
}

// send one packet
void send_packet(struct packet packet, int socket, struct sockaddr* dst_addr){
  spiffy_sendto(socket, &packet, *(unsigned short*)(packet.header+6), 0, dst_addr, sizeof(*dst_addr));
}

// send WHOHAS to all peers
void send_whohas_packet_to_all(struct packet* packets, int packet_count, int socket, struct sockaddr* dst_addr){
  int i = 0;
  for(i=0;i<packet_count;i++){
    send_packet(packets[i], socket, dst_addr);
  }
}

void whohas_flooding(struct Request* request){
  int packet_count = 0;
  struct packet* packets = make_packet(WHOHAS, NULL, NULL, -1, 0, 0, NULL, &packet_count, request);
  if(packet_count!=0){
    struct bt_peer_s* peer = config.peers;
    while(peer!=NULL) {
        if(peer->id==config.identity){
          peer = peer->next;
        }
        else{
          send_whohas_packet_to_all(packets, packet_count, sock, (struct sockaddr*)&peer->addr);
          peer = peer->next;
        }
    }
  }
  free(packets);
  return;
}