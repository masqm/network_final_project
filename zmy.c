#include "zmy.h"

extern struct client_format *client_ptr[MAXCLT];
extern struct client_format client[MAXCLT];
extern struct history_record_format history_record[MAXCLT];

struct group_format group[MAXGROUP];
int groupnum = 0;

char state_name[50][20];


int GroupSearch(char *name, int num) {
	int i = 0;

	if (strlen(name) == 0) {
		return -2;
	}
	if (num == -1)
	{
		for (i = 0; i < 50; i++) {
			if (memcmp(group[i].GroupID, name, 2) == 0) {
				return i;
			}
		}
	}
	else
	{
		for (i = 50; i <= groupnum; i++) {
			if (strcmp(group[i].GroupID, name) == 0) {
				return i;
			}
		}
	}


	return -1;
}

int GroupIntital(char *name) {

	if (groupnum >= MAXGROUP) {
		printf("MAX group num!\n");
		return -1;
	}


	bzero(&group[groupnum], sizeof(struct group_format));

	memcpy(group[groupnum].GroupID, name, 20);
	printf("Group %s created! num: %d\n", name, groupnum);

	groupnum++;
	return (groupnum - 1);
}

int GroupJoin(int gid, char *cname) {
	for (int i = 0; i < group[gid].memnum; i++) {
		if (strcmp(group[gid].members[i], cname) == 0) {
			printf("Member already join");
			return -1;
		}
	}

	memcpy(&group[gid].members[group[gid].memnum], cname, 20);
	printf("Join group %s success, memnum: %d", group[gid].GroupID, group[gid].memnum);

	group[gid].memnum++;
	return group[gid].memnum;
}

int GroupExit(int gid, char *cname) {
	int cid = GMemSearch(gid, cname);
	int i = 0;

	if (cid < 0) {
		return -1;
	}

	bzero(group[gid].members[cid], 20);
	for (i = cid; i < group[gid].memnum; i++) {
		memcpy(group[gid].members[i], group[gid].members[i + 1], 20);
	}
	group[gid].memnum--;


	if (group[gid].memnum == 0) {
		bzero(&group[gid], sizeof(struct group_format));
		for (i = gid; i < groupnum; i++) {
			memcpy(&group[i], &group[i + 1], sizeof(struct group_format));
		}
		groupnum--;
	}

	return cid;
}

int GMemSearch(int gid, char *name) {
	int i = 0;

	if (strlen(name) == 0) {
		return -2;
	}


	for (i = 0; i < group[gid].memnum; i++) {
		if (strcmp(group[gid].members[i], name) == 0) {
			return i;
		}
	}

	return -1;
}

int Gmsg_to_client(int type, int gid, int cid, char *buffer, char *data) {
	struct header_format header;
	int length = 0;
	int nam_len = 0;
	int nowtime = 0;
	char *ptr = buffer + HEAD_SIZE;
	char stamp[20] = { 0 };
	char ck[450] = { 0 };

	int i;

	bzero(&header, HEAD_SIZE);

	switch (type) {
	case 32:
		correct_header(32, group[gid].GroupID, client_ptr[cid]->ClientID, 0, 0, &header);
		hton_header(&header, buffer);

		return HEAD_SIZE;

	case 34:

		if (gid >= 50)
		{
			for (i = 0; i < group[gid].memnum; i++) {
				if (strlen(group[gid].members[i]) != 0) {
					nam_len = strlen(group[gid].members[i]);
					memcpy(ptr, group[gid].members[i], nam_len);
					ptr = ptr + nam_len + 1;
					length = length + nam_len + 1;
				}
			}

			correct_header(34, group[gid].GroupID, client_ptr[cid]->ClientID, length, 0, &header);
		}
		else
		{
			int temp_cid = 0;
			for (i = 1; i < group[gid].memnum; i++) {
				if (strlen(group[gid].members[i]) != 0) {
					temp_cid = find_record(group[gid].members[i]);
					sprintf(ptr, "%s-%s", group[gid].members[i], history_record[temp_cid].profile.city);
					nam_len = strlen(ptr);
					ptr = ptr + nam_len + 1;
					length = length + nam_len + 1;
				}
			}

			correct_header(34, group[gid].GroupID, client_ptr[cid]->ClientID, length, 0, &header);
		}

		hton_header(&header, buffer);
		memcpy(ck, buffer, 450);
		return length;

	case 36:
		//cid is history_record
		nowtime = get_current_time();
		GetTstamp(nowtime, stamp);
		memcpy(ptr, stamp, 20);

		memcpy(ptr + 20, history_record[cid].ClientID, 20);

		length = strlen(data);
		memcpy(ptr + 40, data, length);

		length += 40;

		correct_header(36, group[gid].GroupID, history_record[cid].ClientID, length, nowtime, &header);
		hton_header(&header, buffer);

		return length;

	default:

		break;
	}
}

int GetTstamp(int nowtime, char *stamp) {
	struct tm *tm_t;
	time_t now = nowtime;

	bzero(stamp, 20);

	tm_t = localtime(&now);
	strftime(stamp, 20, "%Y-%m-%d %H:%M:%S", tm_t);
}

int Intital_RGroup()
{
	FILE * fp = fopen("states_b.txt", "r");//opeb states_b
	if (fp == NULL) {
		puts("open erro");
		return -1;
	}
	for (int i = 0; i < 50; i++)
	{
		fgets(state_name[i], 20, fp);
	}
	fclose(fp);

	bzero(group, 50 * sizeof(struct group_format));

	char adm[20] = { 0 };

	sprintf(adm, "Administrator\0");

	for (int i = 0; i < 50; i++)
	{
		memcpy(group[i].GroupID, state_name[i], 20);
		GroupJoin(i, adm);

	}

	groupnum = 50;

	return 1;
}

int Refresh_RGroup(char *now, int filedes)
{
	int c = client_ptr[filedes]->cursor;
	int gid = 0;


	if (strlen(history_record[c].profile.region) != 0)
	{
		gid = GroupSearch(history_record[c].profile.region, -1);
		GroupExit(gid, client_ptr[filedes]->ClientID);
	}

	gid = GroupSearch(now, -1);
	GroupJoin(gid, client_ptr[filedes]->ClientID);
	return gid;

}

