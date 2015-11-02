#include "guess_client.h"
#define MAXBUF 1024

/* Sends guess to server */
void sendguess(char *nick, int s, const struct sockaddr *to, socklen_t tolen){
  char msg[MAXBUF];
  char msgtype[] = "2 ";
  char delim[] = " ";
  strcpy(msg, msgtype);
  strcat(msg, nick);
  char guess[24];

  printf("guess number: ");
  scanf("%s", guess);
  strcat(msg, delim); // add space for protocol
  strcat(msg, guess); // add guess to the message
  strcat(msg, delim); 
  printf("::%s::",msg);
  sendto(s, msg, strlen(msg), 0, to, tolen);
}

/* Receives information from server: what other players have guessed and tells when it is players turn ahain */
void getinfo(int socket,  struct sockaddr *from, socklen_t fromlen, int *turn){

  char recvbuf[MAXBUF];
  recvfrom(socket, recvbuf, MAXBUF-1, 0, from, &fromlen);
  printf("got: %s\n", recvbuf);

  /* check for turn msg */
  if (atoi(recvbuf) == -99){
    *turn = 1;
    printf("Your turn to guess!\n");
  }
}

void join(char *nick, int s, const struct sockaddr *to, socklen_t tolen){
  char msg[MAXBUF];
  char msgtype[] = "1 ";
  strcpy(msg, msgtype);
  strcat(msg, nick);
  printf("::%s::\n", msg);


  sendto(s, msg, strlen(msg), 0, to, tolen);
}

/* send join message/nick to the server and join the game */


int client(char *ip, char *port){
  char nick[8];
  int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	//int numbytes;
  //char msg[24];
  int myturn = 1;

  memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(ip, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}



	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}

  printf("Input nick: ");
  scanf("%s", nick);

  /* send nick as join msg to server */
  join(nick, sockfd, p->ai_addr, p->ai_addrlen);

  /* MAIN LOOP */
  while(1){

    if(myturn){
      sendguess(nick, sockfd, p->ai_addr, p->ai_addrlen);
      //myturn = 0;
    }
    else{
      //getinfo(sockfd, p->ai_addr, p->ai_addrlen, &myturn);
    }




  }

  return 0;
}

int main(int argc, char *argv[]){
  if(argc == 3){
    client(argv[1], argv[2]);
  }
  else{
    printf("Usage: guessclient IP port\n");
  }
}
