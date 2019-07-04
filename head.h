//
// Created by Jinyong on 2019-04-16.
//

#ifndef _HEAD_H
#define _HEAD_H

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
//#include "zmy.h"




#define FPORT     9000
#define PORT     9219
#define MAXCLT   24
#define MAXTMOUT 1000000
#define SELECTTMOUT 5
#define SERVER "Server"

#define HEAD_SIZE 50
#define STAMP_SIZE 20
#define NAME_SIZE 20
#define MAX_RCD 500

//edited on 3.26.2019 by Bob.
#define MAXFILE 100
#define MD5_DIGEST_LENGTH 20



#define MAXGROUP   55
#define MAXGMEM   5
#define MAXGHIS   10

typedef struct group_format {

	int flag;   // -1: incomplete 0: old complete, new begin
	char GroupID[20];
	char members[MAXGMEM][20];
	int memnum;
};

typedef struct __attribute__((__pacaked__)) header_format {


	unsigned short type;
	char source[20];
	char destination[20];
	unsigned int length;
	unsigned int message_id;
};


typedef struct client_format {


	int flag;   // -1: incomplete 0: old complete, new begin
	int last_time;
	int length;   // Sum of header and data
	char buffer[450];
	int ptr;     // point out the
	int cursor;  // point out the sequence number of history record
	char ClientID[20];
	struct header_format *header;

};

typedef struct history_format {

	int length;
	char buffer[450];
};



typedef struct __attribute__((__pacaked__))profile_format {

	char name[20];
	char ip[20];
	char continent[20];
	char country[20];
	char region[20];
	char city[20];
	char language[20];
	char introduction[50];
};

typedef struct history_record_format {

	char ClientID[20];
	int filedes; // > 1 for online; -1 for offline; 0 for default
	struct history_format history_msg[MAX_RCD];
	struct profile_format profile;

};



struct __attribute__((__packed__)) filename {

	char filename[20];
	unsigned char MD5[20];
};

#endif //INC_333_SERVER_HEAD_H
