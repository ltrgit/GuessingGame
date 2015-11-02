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



#endif
