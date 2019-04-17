//
// Created by Jinyong on 2019-04-16.
//

#ifndef _ZMY_H
#define _ZMY_H

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





int GroupSearch(char *name, int num);

int GroupIntital(char *name);

int GroupJoin(int gid, char *cname);

int GroupExit(int gid, char *cname);

int GMemSearch(int gid, char *name);

int Gmsg_to_client(int type, int gid, int cid, char *buffer, char *data);

int GetTstamp(int nowtime, char *stamp);

int Intital_RGroup();
int Refresh_RGroup(char *now, int filedes);


extern int get_current_time();
extern void correct_header(int type, char source[20], char destination[20], unsigned int data_len, unsigned int msg_id,
	struct header_format *header);
extern void hton_header(struct header_format *header, char *buffer_ptr);
extern int find_record(char *client_name);
#endif //INC_333_SERVER_ZMY_H
