#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <openssl/md5.h>
#include <pthread.h>
#include <sys/stat.h>
#include "head.h"
#include "zmy.h"



extern struct group_format group[MAXGROUP];
extern int groupnum;

struct client_format client[MAXCLT];
struct client_format *client_ptr[MAXCLT];
struct history_record_format history_record[MAXCLT];
struct history_record_format *history_record_ptr[MAXCLT];

fd_set active_fd_set, read_fd_set;

//extern int groupnum;

//edited on 3.29.2019 by Bob.

struct filename file_list[MAXFILE];

char *temp_list[2];

//edited bob 4.15.2019
char p2pclient[20] = { 0 };

int tsock;
int tflag = -1;
int dflag = -1;
int pflag = -1; //edited bob 4.15.2019
int download_size = 0;
int file_size = 0;

unsigned char md[MD5_DIGEST_LENGTH];

FILE *fp;
struct stat st;
int sent = 0;

extern char state_name[50][20];

//end of edition

void msg_to_client(int type, char client_id[20], int msg_id, char *buffer);


int get_current_time();


void clean_client_atr(int i);


void write_to_client(int filedes, char *buffer, int length);


int read_from_client(int filedes);


int ck_client_present(char name[20]);


int make_socket(uint16_t port);


void process_msg(int filedes, int length);


void construct_message(struct header_format *header, char *data, int data_length, char *message);


void correct_header(int type, char source[20], char destination[20], unsigned int data_len, unsigned int msg_id,
	struct header_format *header);


int ck_is_correct(struct header_format *header, int length, int filedes);


void *ntoh_header(char *buffer, struct header_format *temp_header);


void hton_header(struct header_format *header, char *buffer_ptr);

void eliminate(int filedes);

int ck_list_length();

int find_record(char *client_name);

int find_client(char *client_name);

int online(char *client_name, int filedes);

void offline(int filedes);

int find_free_record();

void clean_hr_attr(int ptr);

int recall_history(int cursor);

int store_msg(char *buffer, int cursor, int length);

void SearchbyT(char *time, int cursor);

void SearchbyW(char *key, int cursor);

void SearchbyN(char *key, int cursor);


//!!!

void *THR_file_transmit_function(void *ptr);


//!!! Bob

//added by Bob 3.29.2019.
unsigned char * MD5_hash_generator(char *file_name);

void init_file_list();

void free_file_list();

int add_file(char *id);

int remove_file(char *id, unsigned char *file_MD5);

int find_file(char *id, unsigned char *file_MD5);

int ck_file_list_length();

int main(void) {

	//extern int make_socket(uint16_t port);
	int sock;
	int i;
	struct sockaddr_in clientname;
	size_t size;
	struct timeval timeout;


	timeout.tv_sec = SELECTTMOUT;

	Intital_RGroup();

	//add_file("123.txt");

	int client_no = 0;
	for (client_no = 0; client_no < MAXCLT; client_no++) {

		client_ptr[client_no] = &client[client_no];
		bzero(&client[client_no], sizeof(struct client_format));
		client_ptr[client_no]->header = (struct header_format *) malloc(sizeof(struct header_format));

		history_record_ptr[client_no] = &history_record[client_no];
		bzero(&history_record[client_no], sizeof(struct history_record_format));
	}

	temp_list[0] = (char *)malloc(sizeof(char) * 20);
	temp_list[1] = (char *)malloc(sizeof(char) * 20);


	/* Create the socket and set it up to accept connections. */
	sock = make_socket(PORT);
	if (listen(sock, 1) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	//edited sock
//    tsock = make_socket(FPORT);
//
//    if (listen(tsock, 1) < 0) {
//        perror("listen");
//        exit(EXIT_FAILURE);
//    }

//    tsock = socket(AF_INET, SOCK_STREAM, 0);
//
//    dest_addr_t.sin_family = AF_INET;
//    dest_addr_t.sin_port = htons(9000);
//    dest_addr_t.sin_addr.s_addr = inet_addr(inet_ntoa(inAddr));
//
//    bzero(&(dest_addr_t.sin_zero), 8);

	/* Initialize the set of active sockets. */
	FD_ZERO(&active_fd_set);
	FD_SET(sock, &active_fd_set);

	char *message1 = "thread1";
	pthread_t thread1;
	int ret_thrd1 = pthread_create(&thread1, NULL, &THR_file_transmit_function, (void *)message1);

	//edited by Bob on 3.26.2019
	//init_file_list();

	while (1) {
		/* Block until input arrives on one or more active sockets. */
		read_fd_set = active_fd_set;
		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, &timeout) < 0) {
			perror("select");
			exit(EXIT_FAILURE);
		}


		/* Service all the sockets with input pending. */
		for (i = 0; i < FD_SETSIZE; ++i) {
			if (FD_ISSET(i, &read_fd_set)) {
				if (i == sock) {
					/* Connection request on original socket. */
					int newsock;
					size = sizeof(clientname);
					newsock = accept(sock, (struct sockaddr *) &clientname, (socklen_t *)&size);
					fprintf(stderr, "Socket id: %d\n", newsock);
					if (newsock < 0) {
						perror("accept");
						exit(EXIT_FAILURE);
					}
					fprintf(stderr, "Server: connect from host %s, port %hd.\n", inet_ntoa(clientname.sin_addr),
						ntohs(clientname.sin_port));
					FD_SET(newsock, &active_fd_set);


				}
				else {
					/* Data arriving on an already-connected socket. */
					if (read_from_client(i) == 0) {
						//close(i);
						//FD_CLR (i, &active_fd_set);
						//bzero(client_ptr[i], sizeof(struct client_format));
						offline(i);
					}


				}


			}
		}


		for (int ii = 0; ii < MAXCLT; ii++) {
			if ((get_current_time() - client_ptr[ii]->last_time >= MAXTMOUT) && (client_ptr[ii]->last_time != 0)) {
				offline(ii);
			}
		}
	}
}


/*
 * Convert host order to network order and return buffer's pointer.
 */
void hton_header(struct header_format *header, char *buffer_ptr) {


	unsigned short temp_type = htons(header->type);
	unsigned int temp_length = htonl(header->length);
	unsigned int temp_message_id = htonl(header->message_id);


	bzero(buffer_ptr, 50);
	memcpy(buffer_ptr, &temp_type, 2);
	memcpy(buffer_ptr + 2, header->source, 20);
	memcpy(buffer_ptr + 22, header->destination, 20);
	memcpy(buffer_ptr + 42, &temp_length, 4);
	memcpy(buffer_ptr + 46, &temp_message_id, 4);


}


/*
 * Convert network order to host order and return structured header.
 */
void *ntoh_header(char *buffer, struct header_format *temp_header) {


	char temp_source[20];
	char temp_destination[20];
	unsigned short temp_short_buffer = 0;
	unsigned int temp_long_buffer = 0;


	memcpy(temp_source, buffer + 2, 20);
	memcpy(temp_destination, buffer + 22, 20);
	memcpy(temp_header->source, temp_source, 20);
	memcpy(temp_header->destination, temp_destination, 20);


	memcpy(&temp_short_buffer, buffer, 2);
	temp_header->type = ntohs(temp_short_buffer);


	temp_long_buffer = 0;
	memcpy(&temp_long_buffer, buffer + 42, 4);
	temp_header->length = ntohl(temp_long_buffer);


	temp_long_buffer = 0;
	memcpy(&temp_long_buffer, buffer + 46, 4);
	temp_header->message_id = ntohl(temp_long_buffer);


}


/*
 * check if the content from client is correct
 * length: buffer length = 50 + data length
 */
int ck_is_correct(struct header_format *header, int length, int filedes) {

	/*
	if (header->type == 1) {

		if (strcmp(header->destination, SERVER) != 0) {
			fprintf(stderr, "message format is incorrect!\n");
			return -1;
		} else if (header->length != 0 | header->message_id != 0) {
			fprintf(stderr, "message format is incorrect!\n");
			return -1;
		} else if (length != 50) {
			fprintf(stderr, "message format is incorrect!\n");
			return -1;
		} else if (strlen(header->source) == 0 | strlen(header->destination) == 0) {
			// destination or source is empty
			fprintf(stderr, "message format is incorrect!\n");
			return -1;
		} else {
			return 1;
		}

	} else if (header->type == 3) {

		if (strcmp(header->destination, SERVER) != 0) {
			fprintf(stderr, "message format is incorrect!\n");
			return -1;
		} else if (header->length != 0 | header->message_id != 0) {
			fprintf(stderr, "message format is incorrect!\n");
			return -1;
		} else if (length != 50) {
			fprintf(stderr, "message format is incorrect!\n");
			return -1;
		} else if (strcmp(header->source, client_ptr[filedes]->ClientID) != 0) {
			// pretend other client, or client id is empty
			fprintf(stderr, "message format is incorrect!\n");
			return -1;
		} else if (strlen(header->source) == 0 | strlen(header->destination) == 0) {
			// destination or source is empty
			fprintf(stderr, "message format is incorrect!\n");
			return -1;
		} else {
			return 1;
		}

	} else if (header->type == 5) {

		if (header->length == 0 | header->message_id < 1) {
			// chat do not have content, not satisfied message id >= 1
			fprintf(stderr, "message format is incorrect!\n");
			return -1;
		} else if (length == 50) {
			// chat do not have content
			fprintf(stderr, "message format is incorrect!\n");
			return -1;
		} else if (header->length + 50 != length) {
			// length in header is not equal to the length field in header
			fprintf(stderr, "message format is incorrect!\n");
			return -1;
		} else if (strcmp(header->source, client_ptr[filedes]->ClientID) != 0) {
			// pretend other client, or client id is empty
			fprintf(stderr, "message format is incorrect!\n");
			return -1;
		} else if (strlen(header->source) == 0 | strlen(header->destination) == 0) {
			// destination or source is emptyBad address
			fprintf(stderr, "message format is incorrect!\n");
			return -1;
		} else if (header->length > 400) {
			// data length > 400
			fprintf(stderr, "message format is incorrect!\n");
			return -1;
		} else {
			return 1;
		}

	} else if (header->type == 6) {

		if (strcmp(header->destination, SERVER) != 0) {
			fprintf(stderr, "message format is incorrect!\n");
			return -1;
		} else if (header->length != 0 | header->message_id != 0) {
			fprintf(stderr, "message format is incorrect!\n");
			return -1;
		} else if (length != 50) {
			fprintf(stderr, "message format is incorrect!\n");
			return -1;
		} else if (strlen(header->source) == 0 | strlen(header->destination) == 0) {
			// destination or source is empty
			fprintf(stderr, "message format is incorrect!\n");
			return -1;
		} else if (strcmp(header->source, client_ptr[filedes]->ClientID) != 0) {
			// pretend other client, or client id is empty
			fprintf(stderr, "message format is incorrect!\n");
			return -1;
		} else {
			return 1;
		}

	} else if (header->type != 1 && header->type != 3 && header->type != 5 && header->type != 6) {

		// type is not 1, 3, 5, 6
		//Group message
		if () {

		} else if (header->type == 31)//join group
		{
			return 1;
		} else if (header->type == 35)//group message
		{
			return 1;
		} else if (header->type == 33)//group list
		{
			return 1;
		} else if (header->type == 37)//exit group
		{
			return 1;
		}


		return -1;

	}
	*/

	return 1;

}


/*
 * construct header
 */
void correct_header(int type, char source[20], char destination[20], unsigned int data_len, unsigned int msg_id,
	struct header_format *header) {

	bzero(header, sizeof(*header));
	switch (type) {
	case 1:
		header->type = 1;
		memcpy(header->source, source, strlen(source));
		memcpy(header->destination, SERVER, 7);
		header->length = 0;
		header->message_id = 0;
		break;

	case 2:
		header->type = 2;
		memcpy(header->source, SERVER, 7);
		memcpy(header->destination, destination, strlen(destination));
		header->length = 0;
		header->message_id = 0;
		break;
	case 3:
		header->type = 3;
		memcpy(header->source, source, strlen(source));
		memcpy(header->destination, SERVER, 7);
		header->length = 0;
		header->message_id = 0;
		break;
	case 4:
		header->type = 4;
		memcpy(header->source, SERVER, 7);
		memcpy(header->destination, destination, strlen(destination));
		header->length = data_len;
		header->message_id = 0;
		break;
	case 5:
		header->type = 5;
		memcpy(header->source, source, strlen(source));
		memcpy(header->destination, destination, strlen(destination));
		header->length = data_len;
		header->message_id = msg_id;
		break;
	case 6:
		header->type = 6;
		memcpy(header->source, source, strlen(source));
		memcpy(header->destination, SERVER, 7);
		header->length = 0;
		header->message_id = 0;
		break;
	case 7:
		header->type = 7;
		memcpy(header->source, SERVER, 7);
		memcpy(header->destination, destination, strlen(destination));
		header->length = 0;
		header->message_id = 0;
		break;
	case 8:
		header->type = 8;
		memcpy(header->source, SERVER, 7);
		memcpy(header->destination, destination, strlen(destination));
		header->length = 0;
		header->message_id = msg_id;
		break;

	case 32:
		header->type = 32;
		memcpy(header->source, source, 20);
		memcpy(header->destination, destination, 20);
		header->length = 0;
		header->message_id = msg_id;
		break;

	case 34:
		header->type = 34;
		memcpy(header->source, source, 20);
		memcpy(header->destination, destination, 20);
		header->length = data_len;
		header->message_id = msg_id;
		break;

	case 36:
		header->type = 36;
		memcpy(header->source, source, 20);
		memcpy(header->destination, destination, 20);
		header->length = data_len;
		header->message_id = msg_id;
		break;


		//edited on 3.25.2019 by Bob.

	case 19: //file transfer permission
		header->type = 19;
		memcpy(header->source, SERVER, 7);
		memcpy(header->destination, destination, strlen(destination));
		header->length = 0;
		header->message_id = msg_id;
		break;

	case 20: //file download permission
		header->type = 20;
		memcpy(header->source, SERVER, 7);
		memcpy(header->destination, destination, strlen(destination));
		header->length = data_len;
		header->message_id = msg_id;
		break;

	case 25: //file list response
		header->type = 25;
		memcpy(header->source, SERVER, 7);
		memcpy(header->destination, destination, strlen(destination));
		header->length = data_len;
		header->message_id = 0;
		break;

	case 27: //error: file doesn't exist
		header->type = 27;
		memcpy(header->source, SERVER, 7);
		memcpy(header->destination, destination, strlen(destination));
		header->length = 0;
		header->message_id = 0;
		break;

	case 28: //error: file already exists
		header->type = 28;
		memcpy(header->source, SERVER, 7);
		memcpy(header->destination, destination, strlen(destination));
		header->length = 0;
		header->message_id = 0;
		break;

	case 29: //file transfer ack.
		header->type = 29;
		memcpy(header->source, SERVER, 7);
		memcpy(header->destination, destination, strlen(destination));
		header->length = 0;
		header->message_id = 0;
		break;

	case 30: //file transfer complete ack.
		header->type = 29;
		memcpy(header->source, SERVER, 7);
		memcpy(header->destination, destination, strlen(destination));
		header->length = 0;
		header->message_id = 0;
		break;

	case 15:
		header->type = 15;
		memcpy(header->source, SERVER, 7);
		memcpy(header->destination, destination, strlen(destination));
		header->length = data_len;
		header->message_id = 0;
	}
}


/*
 * Use header and data to construct completed message
 */
void construct_message(struct header_format *header, char *data, int data_length, char *message) {


	char header_buffer[50] = { 0 };
	char *hb_ptr = (char *)&header_buffer;
	hton_header(header, hb_ptr);
	memcpy(message, hb_ptr, 50);
	memcpy(message + 50, data, data_length);


}


/*
 * process request and react
 * length = data length + 50
 */
void process_msg(int filedes, int length) {

	if (ck_is_correct(client_ptr[filedes]->header, length, filedes) < 0) {

		// header format is not correct
		fprintf(stderr, "Server: msg format incorrect\n");
		offline(filedes);

	}
	else {

		int type = client_ptr[filedes]->header->type;
		char buff[50] = { 0 };
		char bufff[450] = { 0 };
		char *buffer = buff;
		int length = -1;
		int des = -1;
		int record_ptr = -1;

		char gname[20];
		int gid = -1;
		char *data;
		char name[20] = { 0 };
		//char *name_ptr = name;
		int cid = 0;

		struct profile_format temp_prof;

		//edited by Bob 3.27.2019:
		char *name_bob;
		char *token;
		int file_size;
		int client_sock;
		struct sockaddr_in clientaddr;
		int transfer_sock;
		int n;
		int transfer_port = FPORT;


		int i = 0;
		const char split[2] = "|";
		int temp;
		int clientlen;
		int childfd;
		char key[400] = { 0 };

		bzero(gname, 20);

		//edited bob 4.15.2019
		int p2pfiledes;

		switch (type) {

		case 1:

			if ((ck_client_present(client_ptr[filedes]->header->source) < 0) &&
				(strcmp(client_ptr[filedes]->header->source, client_ptr[filedes]->ClientID) != 0)) {

				// already have this name, return type 7
				buffer = buff;
				bzero(buffer, 50);
				msg_to_client(7, client_ptr[filedes]->header->source, 0, buffer);
				write_to_client(filedes, buffer, 50);
				offline(filedes);
				break;

			}
			else {

				// transmit hello ack
				buffer = buff;
				bzero(buffer, 50);
				memcpy(client_ptr[filedes]->ClientID, client_ptr[filedes]->header->source, 20);
				msg_to_client(2, client_ptr[filedes]->ClientID, 0, buffer);
				write_to_client(filedes, buffer, 50);

				// transmit client list
				length = ck_list_length();
				buffer = bufff;
				bzero(buffer, 450);
				msg_to_client(4, client_ptr[filedes]->ClientID, 0, buffer);
				write_to_client(filedes, buffer, 50 + length);

				clean_client_atr(filedes);
				break;
			}

		case 3:

			if (strlen(client_ptr[filedes]->ClientID) == 0) {
				fprintf(stderr, "Server: clients not register\n");
				offline(filedes);
				break;
			}
			// transmit client list
			buffer = bufff;
			bzero(buffer, 450);
			length = ck_list_length();
			msg_to_client(4, client_ptr[filedes]->ClientID, 0, buffer);
			write_to_client(filedes, buffer, 50 + length);
			clean_client_atr(filedes);
			break;

		case 5:

			if (strlen(client_ptr[filedes]->ClientID) == 0) {
				fprintf(stderr, "Server: clients not register\n");
				offline(filedes);
				break;
			}
			else if (strcmp(client_ptr[filedes]->ClientID, client_ptr[filedes]->header->destination) == 0) {
				// client want to chat with himself
				fprintf(stderr, "Server: clients not allow to chat with himself\n");
				offline(filedes);
				break;
			}

			if (ck_client_present(client_ptr[filedes]->header->destination) > 0) {

				buffer = buff;
				bzero(buffer, 50);
				// send error cannot deliver
				msg_to_client(8, client_ptr[filedes]->ClientID, client_ptr[filedes]->header->message_id, buffer);
				write_to_client(filedes, buffer, 50);
				clean_client_atr(filedes);
				break;
			}

			// transmit buffer data to another client
			des = find_record(client_ptr[filedes]->header->destination);
			write_to_client(des, client_ptr[filedes]->buffer, client_ptr[filedes]->header->length + 50);
			clean_client_atr(filedes);
			break;

		case 6:

			if (strlen(client_ptr[filedes]->ClientID) == 0) {
				fprintf(stderr, "Server: clients not register\n");
				offline(filedes);
				break;
			}
			offline(filedes);
			break;

		case 10:

			//online
			if (online(client_ptr[filedes]->ClientID, filedes) < 0) {
				offline(filedes);
			};

			recall_history(client_ptr[filedes]->cursor);
			clean_client_atr(filedes);
			break;

		case 11:

			// offline
			//clean_client_atr(filedes);
			offline(filedes);
			break;

		case 12:

			// register

			//bzero(&temp_prof, sizeof(profile_format));
			//memcpy(&temp_prof, client_ptr[filedes]->buffer + 50, sizeof(profile_format));

			memcpy(client_ptr[filedes]->ClientID, client_ptr[filedes]->header->source, 20);
			record_ptr = find_record(client_ptr[filedes]->ClientID);
			if (record_ptr == -1) {
				printf("register\n");
				record_ptr = find_free_record();
				clean_hr_attr(record_ptr);
				client_ptr[filedes]->cursor = record_ptr;
				memcpy(history_record_ptr[record_ptr]->ClientID, client_ptr[filedes]->ClientID, 20);
				history_record_ptr[record_ptr]->filedes = filedes;


			}
			else {

				printf("This client already exist, online operation\n");

			}
			//Refresh_RGroup(temp_prof.region, filedes);
			clean_client_atr(filedes);
			break;

		case 13:// update profile

			bzero(&temp_prof, sizeof(struct profile_format));
			memcpy(&temp_prof, client_ptr[filedes]->buffer + 50, sizeof(struct profile_format));
			if (memcmp(&temp_prof, &history_record_ptr[client_ptr[filedes]->cursor]->profile, sizeof(struct profile_format)) == 0)
			{
				//break;
			}
			else
			{
				if (strcmp(history_record_ptr[client_ptr[filedes]->cursor]->profile.region, temp_prof.region) == 0)
				{
					bzero(&history_record_ptr[client_ptr[filedes]->cursor]->profile, sizeof(struct profile_format));
					memcpy(&history_record_ptr[client_ptr[filedes]->cursor]->profile, client_ptr[filedes]->buffer + 50, sizeof(struct profile_format));
				}
				else
				{
					Refresh_RGroup(temp_prof.region, filedes);
					bzero(&history_record_ptr[client_ptr[filedes]->cursor]->profile, sizeof(struct profile_format));
					memcpy(&history_record_ptr[client_ptr[filedes]->cursor]->profile, client_ptr[filedes]->buffer + 50, sizeof(struct profile_format));
				}

			}

			clean_client_atr(filedes);
			break;

		case 14://show profile
			memcpy(name, client_ptr[filedes]->buffer + HEAD_SIZE, 20);

			cid = find_record(name);

			buffer = bufff;
			msg_to_client(15, client_ptr[filedes]->ClientID, 0, buffer);
			memcpy(buffer + HEAD_SIZE, &history_record[cid].profile, sizeof(struct profile_format));

			write_to_client(filedes, buffer, HEAD_SIZE + sizeof(struct profile_format));

			clean_client_atr(filedes);
			break;

		case 31://join

			memcpy(gname, client_ptr[filedes]->buffer + HEAD_SIZE, 20);

			gid = GroupSearch(gname, groupnum);

			if (gid >= 0) {
				GroupJoin(gid, client_ptr[filedes]->ClientID);
			}
			else if (gid == -1) {
				gid = GroupIntital(gname);
				GroupJoin(gid, client_ptr[filedes]->ClientID);
			}

			buffer = buff;
			Gmsg_to_client(32, gid, filedes, buffer, NULL);
			write_to_client(filedes, buffer, HEAD_SIZE);

			clean_client_atr(filedes);
			break;


		case 33://list

			memcpy(gname, client_ptr[filedes]->header->destination, 20);

			if (client_ptr[filedes]->header->message_id == 2)
			{
				gid = GroupSearch(gname, -1);
			}
			else
			{
				gid = GroupSearch(gname, groupnum);
			}


			if (gid >= 0) {
				buffer = bufff;
				length = Gmsg_to_client(34, gid, filedes, buffer, NULL);
				write_to_client(filedes, buffer, HEAD_SIZE + length);
			}

			clean_client_atr(filedes);
			break;

		case 35://message

			memcpy(gname, client_ptr[filedes]->header->destination, 20);
			data = client_ptr[filedes]->buffer + HEAD_SIZE;

			gid = GroupSearch(gname, groupnum);

			if (gid >= 0) {
				int cid = 0;
				//buffer = bufff;

				buffer = bufff;
				bzero(buffer, 450);

				for (int i = 0; i < group[gid].memnum; i++) {
					cid = find_record(group[gid].members[i]);


					if (cid >= 0) {

						length = Gmsg_to_client(36, gid, cid, buffer, data);

						store_msg(buffer, cid, HEAD_SIZE + length);
						if (history_record_ptr[cid]->filedes > 0) {
							write_to_client(history_record_ptr[cid]->filedes, buffer, HEAD_SIZE + length);
						}

					}
				}
			}

			clean_client_atr(filedes);
			break;

		case 37:

			memcpy(gname, client_ptr[filedes]->buffer + HEAD_SIZE, 20);

			gid = GroupSearch(gname, groupnum);

			if (gid >= 0) {
				GroupExit(gid, client_ptr[filedes]->ClientID);
			}

			clean_client_atr(filedes);
			break;

			//edited on 3.25.2019 by Bob.
			// case 20: //file transfer

			//     if (strlen(client_ptr[filedes]->ClientID) == 0) {
			//         fprintf(stderr, "Server: clients not register\n");
			//         close_client(filedes);
			//         break;
			//     }

			//     // first write how to receive the data without using multithread.
			//     // talk to partner tomorrow how to transfer this part to multithread.
			//     if(find_file(upload_name, upload_MD5) == 1) {
			//          int n = fwrite(client_ptr[filedes]->buffer + 50, sizeof(char), client_ptr[filedes]->length, fp);
			//     }
			//     fprintf(stderr, "write %d bytes in file,\n", client_ptr[filedes]->length);
			//     //handle upload file from the sent port number
			//     break;

		case 18: //edit bob 4.15.2019

			if (pflag != -1) {
				break;
			}

			if (strlen(client_ptr[filedes]->ClientID) == 0) {
				fprintf(stderr, "Server: clients not register\n");
				offline(filedes);
				break;
			}

			memcpy(temp_list[0], client_ptr[filedes]->buffer + 50, 20);
			memcpy(temp_list[1], client_ptr[filedes]->buffer + 70, 20);
			memcpy(p2pclient, client_ptr[filedes]->header->destination, 20);

			temp = find_file(temp_list[0], (unsigned char *)temp_list[1]);
			if (temp == 1) {
				msg_to_client(28, client_ptr[filedes]->ClientID, 0, buffer);
				write_to_client(filedes, buffer, 50);
				clean_client_atr(filedes);
				break;
			}
			else {

				FILE* temp;
				temp = fopen(temp_list[0], "r");

				fstat(fileno(temp), &st);
				download_size = st.st_size;

				fclose(temp);

				pflag = 1;
				//continue send the following message while going to do tasks in multi-thread.
				msg_to_client(17, client_ptr[filedes]->ClientID, 9000, buffer);
				for (i = 0; i < MAXCLT; i++)
				{
					if (strcmp(client_ptr[i]->ClientID, p2pclient) == 0) {
						p2pfiledes = i;
					}
				}
				write_to_client(p2pfiledes, buffer, 50 + length);
				clean_client_atr(p2pfiledes);
				break;
			}

		case 21: //file upload request


			if (tflag != -1) {
				break;
			}

			if (strlen(client_ptr[filedes]->ClientID) == 0) {
				fprintf(stderr, "Server: clients not register\n");
				offline(filedes);
				break;
			}

			memcpy(temp_list[0], client_ptr[filedes]->buffer + 50, 20);
			memcpy(temp_list[1], client_ptr[filedes]->buffer + 70, 20);

			//token = strtok(name_bob, split);

//                while (token != NULL) {
//                    strncpy(temp_list[i], token, strlen(token));
//                    i += 1;
//                    token = strtok(NULL, split);
//                }

			temp = find_file(temp_list[0], (unsigned char *)temp_list[1]);
			if (temp == 1) {
				msg_to_client(28, client_ptr[filedes]->ClientID, 0, buffer);
				write_to_client(filedes, buffer, 50);
				clean_client_atr(filedes);
				break;
			}
			else {
				/*
				memcpy(buffer, temp_list[1], strlen(temp_list[1]));
				memcpy(buffer + strlen(temp_list[1]), '|', 1);
				memcpy(buffer + strlen(temp_list[1]) + 1, temp_list[2], strlen(temp_list[2]));
				length = strlen(temp_list[1]) + strlen(temp_list[2]) + 1;
				*/

				tflag = 1;
				msg_to_client(19, client_ptr[filedes]->ClientID, transfer_port, buffer);
				write_to_client(filedes, buffer, 50 + length);
				clean_client_atr(filedes);

				//fp = fopen(upload_name, "w");

				//use a different port and bind with client to do file transfer.

				//clientlen = sizeof(clientaddr);
				/*
				 * accept: wait for client sidet to send connection request
				 */
				 //client_sock = accept(transfer_sock, (struct sockaddr *) &clientaddr, &clientlen);

				 /*
				  * read: read input string from the client
				  */


				break;
			}


		case 22: //file download/transfer request (Done)
			//edited by bob on 4.1.2019

			if (dflag != -1) {
				break;
			}

			if (strlen(client_ptr[filedes]->ClientID) == 0) {
				fprintf(stderr, "Server: clients not register\n");
				offline(filedes);
				break;
			}
			bzero(temp_list[0], 20);
			bzero(temp_list[1], 20);

			memcpy(temp_list[0], client_ptr[filedes]->buffer + 50, 20);
			MD5_hash_generator(temp_list[0]);

			temp = find_file(temp_list[0], md);

			// if file is not found, send error message
			if (temp == 0) {
				msg_to_client(27, client_ptr[filedes]->ClientID, 0, buffer);
				write_to_client(filedes, buffer, 50 + length);
				clean_client_atr(filedes);
				break;
			}
			else { // if file is found, send transfer acknowledgement with new port number.
				printf("what is temp_list[0]: %s\n", temp_list[0]);

				FILE* temp;
				temp = fopen(temp_list[0], "r");

				fstat(fileno(temp), &st);
				download_size = st.st_size;

				fclose(temp);

				dflag = 1;

				//file download permission, type 20.
				msg_to_client(20, client_ptr[filedes]->ClientID, 9000, buffer);
				write_to_client(filedes, buffer, 50);
				clean_client_atr(filedes);

				break;
			}

		case 23: //request to remove file and content. (Done)

			if (strlen(client_ptr[filedes]->ClientID) == 0) {
				fprintf(stderr, "Server: clients not register\n");
				offline(filedes);
				break;
			}

			memcpy(name_bob, client_ptr[filedes]->buffer + 50, length - 50);

			token = strtok(name_bob, split);

			while (token != NULL) {
				strncpy(temp_list[i], token, strlen(token));
				i += 1;
				token = strtok(NULL, split);
			}

			temp = find_file(temp_list[0], (unsigned char *)temp_list[1]);
			// if file is not found, send error message
			if (temp == 0) {
				msg_to_client(27, client_ptr[filedes]->ClientID, 0, buffer);
				write_to_client(filedes, buffer, 50 + length);
				clean_client_atr(filedes);
				break;
			}
			else { // if file is found, send transfer acknowledgement with new port number.
				remove_file(temp_list[0], (unsigned char *)temp_list[1]);
				break;
			}

		case 24: //file list request (done)

			if (strlen(client_ptr[filedes]->ClientID) == 0) {
				fprintf(stderr, "Server: clients not register\n");
				offline(filedes);
				break;
			}

			buffer = bufff;
			bzero(buffer, 500);
			length = ck_file_list_length();
			msg_to_client(25, client_ptr[filedes]->ClientID, 0, buffer);
			write_to_client(filedes, buffer, 50 + length);
			clean_client_atr(filedes);
			break;

		case 26: //file upload success ack.

			if (strlen(client_ptr[filedes]->ClientID) == 0) {
				fprintf(stderr, "Server: clients not register\n");
				offline(filedes);
				break;
			}

			close(client_sock);
			fclose(fp);
			break;

		case 46:
			bzero(key, 400);
			memcpy(key, client_ptr[filedes]->buffer + 50, 20);
			SearchbyW(key, filedes);
			clean_client_atr(filedes);
			break;

		case 47:
			bzero(key, 400);
			memcpy(key, client_ptr[filedes]->buffer + 50, 20);
			SearchbyT(key, filedes);
			clean_client_atr(filedes);
			break;

		case 48:
			bzero(key, 400);
			memcpy(key, client_ptr[filedes]->buffer + 50, 360);
			SearchbyN(key, filedes);
			clean_client_atr(filedes);
			break;

		default:
			fprintf(stderr, "Server: msg invalid\n");
			offline(filedes);
			break;
		}
	}
}


/*
 * create socket
 */
int make_socket(uint16_t port) {
	int sock;
	struct sockaddr_in name;


	/* Create the socket. */
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	/* Give the socket a name. */
	name.sin_family = AF_INET;
	name.sin_port = htons(port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sock, (struct sockaddr *) &name, sizeof(name)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	return sock;
}


/*
 * check if client name is not present
 * -1 for already have this name
 */
int ck_client_present(char name[20]) {


	int i;
	for (i = 0; i < MAXCLT; i++) {
		if (strcmp(client_ptr[i]->ClientID, name) == 0) {
			return -1;
		}
	}
	return 1;
}


/*
 * find empty history record
 * @return ptr of free record; -1 for no empty record
 */
int find_free_record() {

	for (int i = 0; i < MAX_RCD; i++) {

		if (history_record_ptr[i]->filedes == 0) {
			return i;
		}
	}
	return -1;
}


/*
 * read from client
 */
int read_from_client(int filedes) {


	char buff[400] = { 0 };
	char *buffer = buff;
	int nbytes;
	time_t rawTime;
	struct tm now_time;
	time(&rawTime);
	now_time = *(gmtime(&rawTime));
	int age = mktime(&now_time);


	if (client_ptr[filedes]->ptr < 50) {


		nbytes = read(filedes, buffer, 50 - client_ptr[filedes]->ptr);


		if (nbytes < 0) {
			/* Read error. */
			perror("read");
			return 0;
		}
		else if (nbytes == 0) {
			/* End-of-file. */
			fprintf(stderr, "Server: clients closing the socket connection unexpectedly\n");
			return 0;
		}
		else if (nbytes < (50 - client_ptr[filedes]->ptr)) {


			// header is not complete


			client_ptr[filedes]->last_time = mktime(&now_time);
			client_ptr[filedes]->flag = -1;
			memcpy(client_ptr[filedes]->buffer + client_ptr[filedes]->ptr, buffer, nbytes);
			client_ptr[filedes]->ptr = nbytes + client_ptr[filedes]->ptr;
			return 1;


		}
		else if (nbytes == (50 - client_ptr[filedes]->ptr)) {


			// header is complete, wait processing
			memcpy(client_ptr[filedes]->buffer + client_ptr[filedes]->ptr, buffer, nbytes);
			ntoh_header(client_ptr[filedes]->buffer, client_ptr[filedes]->header);
			client_ptr[filedes]->ptr = client_ptr[filedes]->ptr + nbytes;


			if (client_ptr[filedes]->header->length == 0) {


				// no data, directly processing
				process_msg(filedes, 50);
				return 1;
			}


			if (client_ptr[filedes]->header->length > 400) {
				// more than 400
				fprintf(stderr, "Data too large, more than 400 bytes\n");
				return 0;
			}
			// start recv data
			bzero(buffer, 400);
			nbytes = read(filedes, buffer, client_ptr[filedes]->header->length);
			if (nbytes < 0) {
				/* Read error. */
				perror("read");
				return 0;
			}
			else if (nbytes == 0) {
				/* End-of-file. */
				fprintf(stderr, "Server: clients closing the socket connection unexpectedly\n");
				return 0;


			}
			else if (nbytes == client_ptr[filedes]->header->length) {


				// read completely data
				memcpy((client_ptr[filedes]->buffer + client_ptr[filedes]->ptr), buffer, nbytes);
				client_ptr[filedes]->ptr = client_ptr[filedes]->ptr + nbytes;


				//!!!
				process_msg(filedes, client_ptr[filedes]->ptr);
				return 1;


			}
			else if (nbytes < client_ptr[filedes]->header->length) {


				// data part is not complete
				client_ptr[filedes]->last_time = mktime(&now_time);
				client_ptr[filedes]->flag = -1;
				client_ptr[filedes]->length = client_ptr[filedes]->header->length + 50;
				client_ptr[filedes]->ptr = nbytes + 50;
				memcpy(client_ptr[filedes]->buffer + 50, buffer, nbytes);
				return 1;
			}

		}

	}
	else if (client_ptr[filedes]->ptr >= 50) {

		// start recv data
		bzero(buffer, 400);
		nbytes = read(filedes, buffer, client_ptr[filedes]->length - client_ptr[filedes]->ptr);
		if (nbytes < 0) {
			/* Read error. */
			perror("read");
			return 0;
		}
		else if (nbytes == 0) {
			/* End-of-file. */
			fprintf(stderr, "Server: clients closing the socket connection unexpectedly\n");
			return 0;


		}
		else if (nbytes == (client_ptr[filedes]->length - client_ptr[filedes]->ptr)) {


			// read completely data
			memcpy(client_ptr[filedes]->buffer + client_ptr[filedes]->ptr, buffer, nbytes);
			client_ptr[filedes]->ptr = client_ptr[filedes]->ptr + nbytes;


			//!!!
			process_msg(filedes, client_ptr[filedes]->ptr);
			return 1;


		}
		else if (nbytes < (client_ptr[filedes]->length - client_ptr[filedes]->ptr)) {


			// data part is not complete
			client_ptr[filedes]->last_time = mktime(&now_time);
			client_ptr[filedes]->flag = -1;
			memcpy(client_ptr[filedes]->buffer + client_ptr[filedes]->ptr, buffer, nbytes);
			client_ptr[filedes]->ptr = nbytes + client_ptr[filedes]->ptr;
			return 1;
		}


	}
}


/*
 * write to client
 * length = write size
 */
void write_to_client(int filedes, char *buffer, int length) {


	int nbytes;
	nbytes = write(filedes, buffer, length);


	if (nbytes < 0) {
		offline(filedes);
	}


}


/*
 * clean client attribute after processing
 */
void clean_client_atr(int i) {


	bzero(client_ptr[i]->header, sizeof(struct header_format));
	client_ptr[i]->length = 0;
	client_ptr[i]->flag = 0;
	bzero(client_ptr[i]->buffer, 450);
	client_ptr[i]->last_time = 0;
	client_ptr[i]->ptr = 0;
	//bzero(client_ptr[i]->ClientID, 20);
}


/*
 * get current time
 */
int get_current_time() {


	time_t rawTime;
	struct tm now_time;
	time(&rawTime);
	now_time = *(gmtime(&rawTime));
	int age = mktime(&now_time);
	return age;
}


/*
 * construct message to client
 */
void msg_to_client(int type, char client_id[20], int msg_id, char *buffer) {


	struct header_format *header = (struct header_format *) malloc(sizeof(struct header_format));
	int length = 0;
	char *ptr = buffer + 50;


	switch (type) {
	case 2:
		// hello ack
		correct_header(2, SERVER, client_id, 0, 0, header);
		hton_header(header, buffer);
		free(header);
		break;
	case 4:
		// client list
		for (int i = 0; i < MAXCLT; i++) {
			if (strlen(client[i].ClientID) != 0) {
				memcpy(ptr, client[i].ClientID, strlen(client[i].ClientID));
				ptr = ptr + strlen(client[i].ClientID) + 1;
				length = length + strlen(client[i].ClientID) + 1;
			}
		}
		correct_header(4, SERVER, client_id, length, 0, header);
		hton_header(header, buffer);
		free(header);
		break;
	case 7:
		// error: client already present
		correct_header(7, SERVER, client_id, 0, 0, header);
		hton_header(header, buffer);
		free(header);
		break;
	case 8:
		// error: cannot deliver
		correct_header(8, SERVER, client_id, 0, msg_id, header);
		hton_header(header, buffer);
		free(header);
		break;

		// added on 3.29.2019 by Bob

	case 17: //p2p file notification
		printf("notify Clientid of p2p transfer");
		memcpy(ptr, temp_list[0], 20);
		correct_header(17, SERVER, client_id, download_size, msg_id, header);
		hton_header(header, buffer);
		free(header);
		break;

	case 19: //file upload permission
		printf("sending file upload permission\n");
		correct_header(19, SERVER, client_id, 0, msg_id, header);
		hton_header(header, buffer);
		free(header);
		break;

	case 20: //file download permission
		correct_header(20, SERVER, client_id, download_size, msg_id, header);
		hton_header(header, buffer);
		free(header);
		break;

	case 25: //file list response (done)
		for (int i = 0; i < MAXFILE; i++) {
			if (file_list[i].filename != NULL) {
				memcpy(ptr, file_list[i].filename, strlen(file_list[i].filename));
				ptr = ptr + strlen(file_list[i].filename);
				memcpy(ptr, (char *)'|', 1);
				ptr = ptr + 1;
				memcpy(ptr, file_list[i].MD5, strlen((char *)file_list[i].MD5));
				ptr = ptr + strlen((char *)file_list[i].MD5);
				memcpy(ptr, (char *)'|', 1);
				ptr = ptr + 1;
				length = length + strlen(file_list[i].filename) + 2 + strlen((char *)file_list[i].MD5);
			}
		}

		correct_header(25, SERVER, client_id, length, 0, header);
		hton_header(header, buffer);
		free(header);
		break;

	case 27: //error:file_doesn't exist (done)
		printf("error: file_doesn't_exist\n");
		correct_header(27, SERVER, client_id, 0, 0, header);
		hton_header(header, buffer);
		free(header);
		break;

	case 28: //error_file_already_exist (done)
		printf("error: file_already_exist\n");
		correct_header(28, SERVER, client_id, 0, 0, header);
		hton_header(header, buffer);
		free(header);
		break;

	case 29: //file transfer ack (not sure what to do)
		// do a multi-thread here.
		// pthread_t thread1;
		// int ret_thrd1 = pthread_create(&thread1, NULL, &generate_new_port_number, (void *)thread1);

		correct_header(29, SERVER, client_id, 0, 0, header);
		hton_header(header, buffer);
		free(header);
		break;

	case 30:
		printf("file transfer is completed\n");
		correct_header(30, SERVER, client_id, 0, 0, header);
		hton_header(header, buffer);
		free(header);

		fclose(fp);
		sent = 0;
		break;

	case 15:
		correct_header(15, SERVER, client_id, sizeof(struct profile_format), 0, header);
		hton_header(header, buffer);
		free(header);
		break;


	default:
		free(header);
		break;
	}


}


/*
 * return client list length
 */
int ck_list_length() {


	int length = 0;
	for (int i = 0; i < MAXCLT; i++) {


		if (strlen(client[i].ClientID) != 0) {


			length = length + strlen(client[i].ClientID) + 1;
		}
	}


	return length;
}

/*
 * call this function to temporary offline client
 */
void offline(int filedes) {

	printf("History Record [%d] Offline!\n", client_ptr[filedes]->cursor);
	close(filedes);
	FD_CLR(filedes, &active_fd_set);
	history_record_ptr[client_ptr[filedes]->cursor]->filedes = 0;
	clean_client_atr(filedes);
}


/*
 * call this function to temporary awake client
 */
int online(char *client_name, int filedes) {

	int ptr = -1;
	memcpy(client_ptr[filedes]->ClientID, client_ptr[filedes]->header->source, 20);
	ptr = find_record(client_name);
	if (ptr < 0) {
		printf("Exception: Not register client name!\n");
		return -1;
	}
	else {
		client_ptr[filedes]->cursor = ptr;
		history_record_ptr[ptr]->filedes = filedes;
		printf("History Record Client[%d] online\n", ptr);
		return 1;
	}
}


/*
 * eliminate client, stop tracking
 */
void eliminate(int filedes) {


	close(filedes);
	FD_CLR(filedes, &active_fd_set);
	client_ptr[filedes]->length = 0;
	client_ptr[filedes]->cursor = 0;
	client_ptr[filedes]->flag = 0;
	client_ptr[filedes]->last_time = 0;
	bzero(client_ptr[filedes]->header, sizeof(struct header_format));
	bzero(client_ptr[filedes]->ClientID, 20);
	bzero(client_ptr[filedes]->buffer, 450);
	client_ptr[filedes]->ptr = 0;


}


/*
 * from client name to filedes
 */
int find_record(char *client_name) {

	for (int i = 0; i < MAXCLT; i++) {

		if (strcmp(history_record_ptr[i]->ClientID, client_name) == 0)
			//printf("Find History Record[%d]", i);
			return i;
	}
	printf("Find Record Fail\n");
	return -1;
}


/*
 * from client name to filedes
 */
int find_client(char *client_name) {

	int i;
	for (i = 0; i < MAXCLT; i++) {

		if (strcmp(client_ptr[i]->ClientID, client_name) == 0)
			return i;

	}

	return -1;//can't find
}


/*
 * bzero the history record
 */
void clean_hr_attr(int ptr) {

	bzero(history_record_ptr[ptr]->ClientID, 20);
	history_record_ptr[ptr]->filedes = 0;
	bzero(&history_record_ptr[ptr]->profile, sizeof(struct profile_format));

	bzero(history_record_ptr[ptr], sizeof(struct history_record_format));
	//bzero(history_record_ptr[ptr]->history_msg, sizeof(struct history_msg_format));
}



void *THR_file_transmit_function(void *ptr) {

	int newtsock = 0;
	struct sockaddr_in dest_addr_t;
	int size = sizeof(dest_addr_t);
	while (1) {
		sleep(3);

		if (tflag == 1) {

			//            /* Create the socket. */
			//            tsock = socket(AF_INET, SOCK_STREAM, 0);
			//            if (tsock < 0) {
			//                perror("socket");
			//                exit(EXIT_FAILURE);
			//            }
			//
			//            /* Give the socket a name. */
			//            dest_addr_t.sin_family = AF_INET;
			//            dest_addr_t.sin_port = htons(FPORT);
			//            dest_addr_t.sin_addr.s_addr = htonl(INADDR_ANY);
			//            if (bind(tsock, (struct sockaddr *) &dest_addr_t, sizeof(dest_addr_t)) < 0) {
			//                perror("bind");
			//                exit(EXIT_FAILURE);
			//            }

			tsock = make_socket(FPORT);

			if (listen(tsock, 1) < 0) {
				perror("listen");
				exit(EXIT_FAILURE);
			}

			newtsock = accept(tsock, (struct sockaddr *) &dest_addr_t, (socklen_t *)&size);

			fp = fopen(temp_list[0], "wb+");

			//FILE *fpp = fopen("2.txt", "wb+");
			//fwrite("12345", 1, 3, fpp);


			int file_size = 20000;
			char *buf_send = (char *)malloc(sizeof(char) * file_size);
			int n = read(newtsock, buf_send, file_size);
			if (n < 0)
				printf("Error, Read Error \n");

			//n = fwrite("test", 1, 3, fp);

			n = fwrite(buf_send, 1, n, fp);

			fclose(fp);

			int retval = add_file(temp_list[0]);
			if (retval != 1) {
				printf("there is an error adding the file\n");
			}

			sleep(1);

			close(newtsock);

			sleep(1);
			close(tsock);

			tflag = -1;
		}

		if (dflag == 1) {
			printf("what is file:%s\n", temp_list[0]);

			//            /* Create the socket. */
			//            tsock = socket(PF_INET, SOCK_STREAM, 0);
			//            if (tsock < 0) {
			//                perror("socket");
			//                exit(EXIT_FAILURE);
			//            }
			//
			//
			//            /* Give the socket a name. */
			//            dest_addr_t.sin_family = AF_INET;
			//            dest_addr_t.sin_port = htons(9002);
			//            dest_addr_t.sin_addr.s_addr = htonl(INADDR_ANY);
			//            if (bind(tsock, (struct sockaddr *) &dest_addr_t, sizeof(dest_addr_t)) < 0) {
			//                perror("bind");
			//                exit(EXIT_FAILURE);
			//            }

			tsock = make_socket(9002);

			if (listen(tsock, 1) < 0) {
				perror("listen");
				exit(EXIT_FAILURE);
			}

			newtsock = accept(tsock, (struct sockaddr *) &dest_addr_t, (socklen_t *)&size);
			if (newtsock < 0)
				printf("ERROR on accept\n");

			fp = fopen(temp_list[0], "r");

			fstat(fileno(fp), &st);
			file_size = st.st_size;
			printf("what is file size: %d\n", file_size);
			//send file content to client

			char *buf_send = (char*)malloc(sizeof(char) * file_size);
			fread(buf_send, 1, file_size, fp);

			int n = send(newtsock, buf_send, file_size, 0);
			if (n < 0)
				printf("ERROR writing on socket\n");

			//sleep(2);
			fclose(fp);
			sleep(1);

			close(newtsock);

			sleep(1);
			close(tsock);
			dflag = -1;

			//file download complete ack, should be sent in thread.
			/*
			msg_to_client(30, client_ptr[filedes]->ClientID, 0, buffer);
			write_to_client(filedes, buffer, 50);
			clean_client_atr(filedes);
			 */
		}

		if (pflag == 1) {
			printf("what is the file to send: %s, what is the client ID: %s", temp_list[0], p2pclient);

			//first figure out which socket to use to send the file
			//then send the file
			newtsock = accept(tsock, (struct sockaddr *) &dest_addr_t, (socklen_t *)&size);
			if (newtsock < 0)
				printf("ERROR on accept\n");

			fp = fopen(temp_list[0], "r");

			fstat(fileno(fp), &st);
			file_size = st.st_size;
			printf("what is file size: %d\n", file_size);
			//send file content to client

			char *buf_send = (char *)malloc(sizeof(char) * file_size);
			fread(buf_send, 1, file_size, fp);

			int n = send(newtsock, buf_send, file_size, 0);
			if (n < 0)
				printf("ERROR writing on socket\n");

			fclose(fp);
			pflag = -1;
		}
	}

}


/*
 * added on 3.25.2019 by Bob
 */

 /****************************************************
 *
 * MD5 hash generator
 *
 *****************************************************/

unsigned char * MD5_hash_generator(char *file_name) {

	char *filename = file_name;
	int i;
	FILE *inFile = fopen(filename, "rb");
	MD5_CTX mdContext;
	int bytes;
	unsigned char data[1024];

	bzero(md, MD5_DIGEST_LENGTH);
	if (inFile == NULL) {
		printf("%s can't be opened.\n", filename);
		return 0;
	}

	MD5_Init(&mdContext); //initializes a MD5_CTX structures.
	while ((bytes = fread(data, 1, 1024, inFile)) != 0) //called repeatedly via while loop to hash the entire file.
		MD5_Update(&mdContext, data, bytes);
	// places the message digest in md(unsigned char), which must have space for MD5_DIGEST_LENGTH == 16 bytes of output,
	// and erases the MD5_CTX.
	MD5_Final(md, &mdContext);
	for (i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", md[i]); //print out the MD5 hash.
	printf(" %s\n", filename);
	fclose(inFile);
	return md;
}

/****************************************************
*
* File list constructor.
*
*****************************************************/

/*
void init_file_list() {
	for (int i = 0; i < MAXFILE; i++)
	{
		file_list[i].filename = (char *)malloc(sizeof(char) * 20);
		file_list[i].MD5 = malloc(sizeof(char) * 20);
	}
}*/

void free_file_list() {
	for (int i = 0; i < MAXFILE; i++) {
		if (strlen((char*)file_list[i].MD5) == 0) {
			free(file_list[i].filename);
			free(file_list[i].MD5);
		}
	}
}

// return: 1-file added; 0-no empty spot
int add_file(char *id) {
	fprintf(stderr, "adding file: %s\n", id);
	for (int i = 0; i < MAXFILE; i++) {
		if (strlen((char*)file_list[i].MD5) == 0) {

			memcpy(file_list[i].filename, id, 20);
			MD5_hash_generator(id);
			memcpy(file_list[i].MD5, md, 20);
			return 1;
		}
	}
	return 0;
}

// return: 1-file removed; 0-file not found
int remove_file(char *id, unsigned char *file_MD5) {
	for (int i = 0; i < MAXFILE; i++) {
		if (strcmp(file_list[i].filename, id) == 0 && strcmp((char*)file_list[i].MD5, (char*)file_MD5) == 0) {
			bzero(file_list[i].filename, 20);
			bzero(file_list[i].MD5, 20);
			return 1;
		}
	}
	return 0;
}

// return: 1-file found; 0-file not found
int find_file(char *id, unsigned char *file_MD5) {
	fprintf(stderr, "checking client: %s, File MD5: %s\n", id, file_MD5);
	if (id == NULL || strlen(id) == 0)
		return 0;

	for (int i = 0; i < MAXFILE; i++) {
		if (strlen(file_list[i].filename) != 0 && strcmp(id, file_list[i].filename) == 0 &&
			strcmp((char*)file_MD5, (char*)file_list[i].MD5) == 0) {
			fprintf(stderr, "%s: found\n", id);
			return 1;
		}
	}

	fprintf(stderr, "%s doesn't exits\n", id);
	return 0;
}

/*
 * return file_list length
 */
int ck_file_list_length() {

	int length = 0;
	for (int i = 0; i < MAXFILE; i++) {

		if (file_list[i].filename != NULL) {

			length = length + strlen(file_list[i].filename) + 1 + strlen((char*)file_list[i].MD5) + 1;
		}
	}

	return length;
}

/*
 * Store history message
 */
int store_msg(char *buffer, int cursor, int length) {

	//char test[450]={0};

	//memcpy(test, buffer, 450);
	for (int i = 0; i < MAX_RCD; i++) {

		if (history_record_ptr[cursor]->history_msg[i].length == 0) {
			memcpy(history_record_ptr[cursor]->history_msg[i].buffer, buffer, 450);
			history_record_ptr[cursor]->history_msg[i].length = length;
			return 1;
		}
	}
	return -1;
}


/*
 * recall 10 recent msg
 */
int recall_history(int cursor) {
	int i;
	for (i = 0; i < 10; i++) {
		if (history_record_ptr[cursor]->history_msg[i].length != 0) {
			write_to_client(history_record_ptr[cursor]->filedes, history_record_ptr[cursor]->history_msg[i].buffer,
				history_record_ptr[cursor]->history_msg[i].length);
		}
		else {
			return i;
		}
	}
	return i;
}

/*
 * send history msg by keyword
 */
void SearchbyW(char *key, int cursor) {

	char mt_key[360] = { 0 };
	int i;
	char temp[360] = { 0 };
	memcpy(mt_key, key, 360);
	for (i = 0; i < 10; i++) {
		bzero(temp, 360);
		memcpy(temp, history_record_ptr[client_ptr[cursor]->cursor]->history_msg[i].buffer + 90, 360);
		if (history_record_ptr[client_ptr[cursor]->cursor]->history_msg[i].length != 0 &&
			strstr(temp, mt_key) != NULL) {
			write_to_client(history_record_ptr[client_ptr[cursor]->cursor]->filedes, history_record_ptr[client_ptr[cursor]->cursor]->history_msg[i].buffer,
				history_record_ptr[client_ptr[cursor]->cursor]->history_msg[i].length);
			sleep(1);
		}
	}
}


/*
 * send history msg by time
 */
void SearchbyT(char *key, int cursor) {

	char mt_key[20] = { 0 };
	int i;
	char temp[20] = { 0 };
	memcpy(mt_key, key, 20);
	for (i = 0; i < 10; i++) {
		bzero(temp, 20);
		memcpy(temp, history_record_ptr[client_ptr[cursor]->cursor]->history_msg[i].buffer + 50, 20);
		if (history_record_ptr[client_ptr[cursor]->cursor]->history_msg[i].length != 0 &&
			strstr(temp, mt_key) != NULL) {
			write_to_client(history_record_ptr[client_ptr[cursor]->cursor]->filedes, history_record_ptr[client_ptr[cursor]->cursor]->history_msg[i].buffer,
				history_record_ptr[client_ptr[cursor]->cursor]->history_msg[i].length);
			sleep(1);
		}
	}
}


/*
 * send history msg by client name
 */
void SearchbyN(char *key, int cursor) {

	char mt_key[20] = { 0 };
	int i;
	char temp[20] = { 0 };
	memcpy(mt_key, key, 20);
	for (i = 0; i < 10; i++) {
		bzero(temp, 20);
		memcpy(temp, history_record_ptr[client_ptr[cursor]->cursor]->history_msg[i].buffer + 70, 20);
		if (history_record_ptr[client_ptr[cursor]->cursor]->history_msg[i].length != 0 &&
			strstr(temp, mt_key) != NULL) {
			write_to_client(history_record_ptr[client_ptr[cursor]->cursor]->filedes, history_record_ptr[client_ptr[cursor]->cursor]->history_msg[i].buffer,
				history_record_ptr[client_ptr[cursor]->cursor]->history_msg[i].length);
			sleep(1);
		}
	}
}
