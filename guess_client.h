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



int client(char* port, char* ip);
void sendnumber(int s, const struct sockaddr *to, socklen_t tolen);
void addplayerTCP(int fd, struct sockaddr *ip, socklen_t len);


#endif
