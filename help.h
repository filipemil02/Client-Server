#include <vector>

using namespace std;

#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>


#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLEN		256	// dimensiunea maxima a calupului de date
#define MAX_CLIENTS	5	// numarul maxim de clienti in asteptare

struct tcp_message{
	char ip[16];
	char port[10];
	char topic[51];
	char type[10];
	char payload[1501];
};

struct message{
	char id[10];
	char topic[50];
	char type[20];
	int sf;
};

struct topic {
	char name[50];
	int sf;
};

struct client {
	char id[10];
	int active;
	int fd;
	vector <tcp_message> unsentMessages;
	vector <topic> topics;
};

int sendMsg(int socket, struct tcp_message message){
    int n = send(socket, (char *)&message, sizeof(tcp_message), 0);
    DIE(n < 0, "send");
    return n;
}

int sendMsgm(int socket, struct message message){
    int n = send(socket, &message, sizeof(message), 0);
    DIE(n < 0, "send");
    return n;
}

#endif
