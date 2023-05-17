#include <iostream>
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include "help.h"

using namespace std;

void selectSock(int socket, fd_set &fd)
{
	int ret = select(socket, &fd, NULL, NULL, NULL);
	DIE(ret < 0, "error selecting");
}

void connectSock(int socket, sockaddr *address, int size)
{
	int ret = connect(socket, address, size);
	DIE(ret < 0, "error connecting the address to the socket");
}

void sendToServer(int socket, message &msg, int size)
{
	int n = send(socket, &msg, size, 0);
	DIE(n < 0, "error sending message to the server");
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int i, n, flag = 1;

	char buffer[BUFLEN];
	struct sockaddr_in serv_addr;
	int socklen = sizeof(serv_addr);
	int port = atoi(argv[3]);

	int new_sock = socket(AF_INET, SOCK_STREAM, 0);
	DIE(new_sock < 0, "error creating UDP socket");
	memset((char *)&serv_addr, 0, socklen);
	serv_addr.sin_port = htons(port);
	serv_addr.sin_family = AF_INET;
	int ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "error setting server address");

	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(new_sock, &read_fds);

	fd_set tmp_fds;
	FD_ZERO(&tmp_fds);

	connectSock(new_sock, (struct sockaddr *)&serv_addr, socklen);

	n = send(new_sock, argv[1], strlen(argv[1]), 0);
	DIE(n < 0, "Could not send the client ID");

	setsockopt(new_sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));

	while (1)
	{
		tmp_fds = read_fds;

		selectSock(new_sock + 1, tmp_fds);

		if (FD_ISSET(STDIN_FILENO, &tmp_fds))
		{
			message msg;
			memset(&msg, 0, sizeof(message));
			memset(buffer, 0, BUFLEN);
			strcpy(msg.id, argv[1]);

			fgets(buffer, BUFLEN - 1, stdin);

			if (strncmp(buffer, "subscribe", 9) == 0)
			{
				char *subscribe;
				subscribe = strtok(buffer, " ");
				strcpy(msg.type, subscribe);

				char *topic;
				topic = strtok(NULL, " ");
				DIE(topic == NULL, "no topic typed");
				strcpy(msg.topic, topic);

				char *sf;
				sf = strtok(NULL, " ");
				DIE(sf == NULL, "no sf typed");
				DIE(atoi(sf) != 1 && atoi(sf) != 0, "error sf");
				msg.sf = atoi(sf);

				sendToServer(new_sock, msg, sizeof(message));
				printf("Subscribed to topic\n");
			}
			else if (strncmp(buffer, "unsubscribe", 11) == 0)
			{
				char *unsubscribe;
				unsubscribe = strtok(buffer, " ");
				strcpy(msg.type, unsubscribe);

				char *topic;
				topic = strtok(NULL, " \n");
				DIE(topic == NULL, "no topic typed");
				strcpy(msg.topic, topic);

				sendToServer(new_sock, msg, sizeof(message));
				printf("Unsubscribed from topic\n");
			}
			else if (strncmp(buffer, "exit", 4) == 0)
			{
				memset(msg.type, 0, 4);
				strncpy(msg.type, buffer, 4);

				sendMsgm(new_sock, msg);
				return 0;
			}
		}

		if (FD_ISSET(new_sock, &tmp_fds))
		{
			tcp_message tcp_msg;

			n = recv(new_sock, &tcp_msg, sizeof(tcp_message), 0);
			DIE(n <= 0, "error receiving messages from the server");

			printf("%s:%s - %s - %s - ",tcp_msg.ip,tcp_msg.port, tcp_msg.topic, tcp_msg.type);
			if (strcmp(tcp_msg.type, "STRING") == 0)
				printf("%s\n", tcp_msg.payload);
			else
				printf("%d\n", atoi(tcp_msg.payload));
		}
	}
	return 0;
}