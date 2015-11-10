#ifndef __HELLOUDP_H
#define __HELLOUDP_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int server(char*);
void *get_in_addr(struct sockaddr *sa);
void setnumber(char *msg);
void broadcastGuess(int sockfd, char *msg, int len);
void addplayerTCP(int fd, struct sockaddr *ip, socklen_t len);


void askfornum(int socket, const struct sockaddr *to, socklen_t tolen);



#endif
