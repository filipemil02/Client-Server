#include <iostream>
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include "help.h"

using namespace std;

void closeSockets(int tcp, int udp)
{
    close(tcp);
    close(udp);
}

void sendMessage(vector<client> clients, tcp_message msg, unsigned int i, unsigned int j)
{
    if (clients[i].active == true)
        if (strcmp(clients[i].topics[j].name, msg.topic) == 0)
        {
            int n = send(clients[i].fd, (char *)&msg, sizeof(tcp_message), 0);
            DIE(n < 0, "error sending messages to TCP client");
        }
}

int functionInt(char *buffer)
{
    int32_t number = htonl(*(uint32_t *)(buffer + 52));
    if (buffer[51] == 1)
        number = -number;
    return number;
}

float functionFloat(char *buffer)
{
    float number = ntohs(*(uint16_t *)(buffer + 51));
    number = number / (float)(100 * 1.0);
    return number;
}

float functionFloat2(char *buffer)
{
    int sign = *(buffer + 51);
    float number = ntohl(*(uint32_t *)(buffer + 52));
    int e = (int)*(buffer + 56);
    number = number / pow(10, e);
    return number;
}

int receiveMessage(message &msg, int &i)
{
    int n = recv(i, &msg, sizeof(message), 0);
    DIE(n < 0, "error receiving data from clients");
    return n;
}

int acceptSock(int i, sockaddr *client_addr, socklen_t &size)
{
    int socket = accept(i, client_addr, &size);
    DIE(socket < 0, "wrong connection");
    return socket;
}

int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int port = atoi(argv[1]);
    socklen_t socklen = sizeof(struct sockaddr);

    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_sock < 0, "socket tcp");
    struct sockaddr_in tcp_addr;
    memset((char *)&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_port = htons(port);
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    int ret = bind(tcp_sock, (struct sockaddr *)&tcp_addr, socklen);
    DIE(ret < 0, "error binding tcp socket");
    ret = listen(tcp_sock, MAX_CLIENTS);
    DIE(ret < 0, "error listening to tcp clients");

    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(udp_sock < 0, "socket udp");
    struct sockaddr_in udp_addr;
    memset((char *)&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_port = htons(port);
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(udp_sock, (struct sockaddr *)&udp_addr, socklen);
    DIE(ret < 0, "error binding udp socket");

    fd_set read_fds;
    fd_set tmp_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
    FD_SET(tcp_sock, &read_fds);
    FD_SET(udp_sock, &read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(0, &read_fds);

    int max_sock = tcp_sock > udp_sock ? tcp_sock : udp_sock;
    int socket, i, n, flag = 1;
    char buffer[BUFLEN];
    struct sockaddr_in client_addr;
    socklen_t clilen = sizeof(client_addr);
    message message;
    tcp_message tcp_m;
    vector<client> clients;

    setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

    while (1)
    {
        tmp_fds = read_fds;

        ret = select(max_sock + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "wrong socket");

        memset(buffer, 0, BUFLEN);
        i = 0;
        while (i <= max_sock)
        {
            if (FD_ISSET(i, &tmp_fds))
            {
                if (i == tcp_sock)
                {
                    socket = acceptSock(i, (struct sockaddr *)&client_addr, clilen);

                    FD_SET(socket, &read_fds);

                    ret = recv(socket, buffer, BUFLEN - 1, 0);
                    DIE(ret < 0, "error receiving id from client");

                    int connected = 0;

                    unsigned int j = 0;
                    while (j < clients.size())
                    {
                        if (clients[j].active == true)
                        {
                            if (strcmp(clients[j].id, buffer) == 0)
                            {
                                connected = 1;
                                printf("Client %s already connected.\n", buffer);
                                close(socket);
                            }
                        }
                        if (clients[j].active == false)
                        {
                            if (strcmp(clients[j].id, buffer) == 0)
                            {
                                connected = 1;
                                char *address = inet_ntoa(client_addr.sin_addr);
                                int port = htons(client_addr.sin_port);
                                printf("New client %s connected from %s:%hu.\n", clients[j].id, address, port);

                                if (!clients[j].unsentMessages.empty())
                                {
                                    for (auto message : clients[j].unsentMessages)
                                        n = sendMsg(socket, (struct tcp_message)message);
                                    clients[j].unsentMessages.clear();
                                }
                                clients[j].active = true;
                            }
                        }
                        j++;
                    }
                    if (!connected)
                    {
                        max_sock = max(max_sock, socket);

                        client new_client;
                        memset(&new_client, 0, sizeof(client));
                        strcpy(new_client.id, buffer);
                        new_client.active = true;
                        new_client.fd = socket;

                        clients.push_back(new_client);
                        char *address = inet_ntoa(client_addr.sin_addr);
                        int port = htons(client_addr.sin_port);
                        printf("New client %s connected from %s:%hu.\n", new_client.id, address, port);
                    }
                }
                else if (i == udp_sock)
                {
                    memset(buffer, 0, BUFLEN);
                    int size = sizeof(buffer);
                    n = recvfrom(udp_sock, buffer, size, 0, (struct sockaddr *)&client_addr, &clilen);
                    DIE(n < 0, "error receiving UDP message");

                    memset(&tcp_m, 0, sizeof(tcp_message));
                    char *address = inet_ntoa(client_addr.sin_addr);
                    strcpy(tcp_m.ip, address);
                    sprintf(tcp_m.port, "%u", (unsigned int)ntohs(client_addr.sin_port));
                    strcpy(tcp_m.topic, buffer);

                    unsigned int type = buffer[50];
                    DIE((type > 3 || type < 0), "error type");

                    if (type == 0)
                    {
                        int number = functionInt(buffer);
                        sprintf(tcp_m.payload, "%d", (int)number);
                        strcpy(tcp_m.type, "INT");
                    }
                    else if (type == 1)
                    {
                        float number = functionFloat(buffer);
                        sprintf(tcp_m.payload, "%.2f", number);
                        strcpy(tcp_m.type, "SHORT_REAL");
                    }
                    else if (type == 2)
                    {
                        float number = functionFloat2(buffer);
                        if (*(buffer + 51) == 1)
                            number = -number;
                        sprintf(tcp_m.payload, "%lf", number);
                        strcpy(tcp_m.type, "FLOAT");
                    }
                    else
                    {
                        strcpy(tcp_m.payload, (buffer + 51));
                        strcpy(tcp_m.type, "STRING");
                    }
                    unsigned int j = 0;
                    while (j < clients.size())
                    {
                        unsigned int k = 0;
                        while (k < clients[j].topics.size())
                        {
                            if (clients[j].active == false)
                                if (strcmp(clients[j].topics[k].name, tcp_m.topic) == 0)
                                    if (clients[j].topics[k].sf == 1)
                                        clients[j].unsentMessages.push_back(tcp_m);
                            sendMessage(clients, tcp_m, j, k);
                            k++;
                        }
                        j++;
                    }
                }
                else if (i == STDIN_FILENO)
                {
                    memset(buffer, 0, BUFLEN);
                    fgets(buffer, BUFLEN - 1, stdin);
                    int x = strncmp(buffer, "exit\n", 5);
                    if (x == 0)
                    {
                        for (int j = 0; j <= max_sock; j++)
                            close(j);
                        closeSockets(tcp_sock, udp_sock);
                        return 0;
                    }
                }
                else
                {
                    memset(&message, 0, sizeof(message));
                    int t = i;
                    n = receiveMessage(message, i);

                    if (n == 0)
                    {
                        FD_CLR(i, &read_fds);
                        close(i);
                    }
                    int index;
                    unsigned int j = 0;
                    while (j < clients.size())
                    {
                        if (strcmp(clients[j].id, message.id) == 0)
                        {
                            index = j;
                        }
                        j++;
                    }

                    if (strncmp(message.type, "exit", 4) == 0)
                    {
                        clients[index].active = false;
                        printf("Client %s disconnected.\n", message.id);
                    }
                    if (strncmp(message.type, "subscribe", 9) == 0)
                    {
                        bool topic_exist = false;
                        for (const auto &topic : clients[index].topics)
                        {
                            if (topic.name == message.topic)
                            {
                                topic_exist = true;
                                break;
                            }
                        }

                        if (!topic_exist)
                        {
                            topic new_topic;
                            new_topic.sf = message.sf;
                            strcpy(new_topic.name, message.topic);
                            clients[index].topics.push_back(new_topic);
                        }
                    }
                    if (strncmp(message.type, "unsubscribe", 11) == 0)
                        for (auto it = clients[index].topics.begin(); it != clients[index].topics.end();)
                        {
                            if (strcmp((*it).name, message.topic) == 0)
                                clients[index].topics.erase(it);
                            it++;
                        }
                }
            }
            i++;
        }
    }
    return 0;
}