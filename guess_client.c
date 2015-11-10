#include "guess_client.h"
#define MAXBUF 1024
#define TCPPORT "53100"

/* WARNING GLOBALS */
int isMyTurn = 1; /* Is my turn? */
int won = 0;      /* Did I win? */
int chatturn = 0; /* Guess or chat */


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
  //printf("::%s::",msg);
  sendto(s, msg, strlen(msg), 0, to, tolen);
}

/* set/send new number to the server which other players can guess */
void sendnumber(int s, const struct sockaddr *to, socklen_t tolen){
  char msg[MAXBUF];
  char msgtype[MAXBUF] = "3 "; /* msg type for set new number */

  printf("%s\n", "Input new number to guess: ");
  scanf("%s\n", msg);

  /* Catenate type and new number and send it to the server */
  strcat(msgtype, msg);
  //printf("setnum: --%s--\n", msgtype);
  sendto(s, msgtype, strlen(msgtype), 0, to, tolen);

}

/* Receives information from server: what other players have guessed and tells when it is players turn ahain */
void getinfo(int socket,  struct sockaddr *from, socklen_t fromlen){

  int numbytes;
  char recvbuf[MAXBUF];
  char guess[MAXBUF];
  char tmpmsg[MAXBUF]; // copy of original msg, meeder for strtok()
  char *type; // strtok packets firs part to see what kind of message it is
  char *pmsg;
  memset(recvbuf, '\0', sizeof(recvbuf));

  numbytes = recvfrom(socket, recvbuf, MAXBUF-1, 0, from, &fromlen);
  //printf("got: %s\n", recvbuf);
  recvbuf[numbytes] = '\0';

  /* copy msg to tmpmsg so that strtok won't affect the original msg */
  strcpy(tmpmsg, recvbuf);
  //printf("GOT: %s\n", tmpmsg);
  type = strtok(tmpmsg, " ");   /* get msgtype */

  /* Act according to msg type */
  /* if msgtype = -99 player can set the new number */
  if (atoi(recvbuf) == -99){
    won = 1;
    printf("%s\n", "Winner winner chicken dinner!\n\n");
  }
  /* other players guess */
  else if(atoi(type) == -16){
    pmsg = strtok(NULL, " ");
    //printf("%s\n", recvbuf); /* tulee okein */
    strcpy(guess, pmsg);
    strcat(guess, " guessed ");
    pmsg = strtok(NULL, " ");
    strcat(guess, pmsg);
    printf("%s\n", guess);
  }
}

/* From Beej's guide: http://beej.us/guide/bgnet/output/html/multipage/advanced.html#sendall  */
int sendAllTCP(int socket, char *buf, int *len){
  int sent = 0;   /* how many bytes sent */
  int bytesleft = *len; /* how many bytes yet be sent */
  int numb = 0;

  while (sent < *len) {
    numb = send(socket, buf+sent, bytesleft, 0);
    if (numb == -1){
      break;
    }
    sent += numb;
    bytesleft -= numb;
  }
  *len = sent;
  printf("%s\n", "Sent nick\n");
  return numb==-1?-1:0;
}

/* Sends a join msg to the server and nick along with it */
void join(char *nick, int s, const struct sockaddr *to, socklen_t tolen){
  char msg[MAXBUF];
  char msgtype[] = "1 ";
  strcpy(msg, msgtype);
  strcat(msg, nick);
  //printf("::%s::\n", msg);


  sendto(s, msg, strlen(msg), 0, to, tolen);
}

void TCPjoin(int sockfd, char *nick){
  int len = strlen(nick);

  if (sendAllTCP(sockfd, nick, &len) == -1){
    perror("Send all error!");
    printf("%s: %d bytes\n", "only sent ", len);
  }
}

void Chat(int tcpsock){
  int len = 0;
  char chatmsg[MAXBUF];
  printf("%s", "Input chat message: ");
  scanf("%s", chatmsg);
  len = strlen(chatmsg);
  if ((sendAllTCP(tcpsock, chatmsg, &len)) == -1){
    perror("Chat sendall error");
  }
}

/* send join message/nick to the server and join the game */


int client(char *ip, char *port){
  char nick[8];
  int sockfd;
  int tcpsockfd;
	struct addrinfo hints, *servinfo, *p, *res, *ai;
	int rv;
	//int numbytes;
  //char msg[24];
  int fdmax;
  fd_set readfds, writefds, master;
  char buf[MAXBUF];
  int nbytes;
  char chatmsg[MAXBUF];

  /* For UDP connection / gameplay */
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

  /* For TCP chat connection */
  memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(ip, TCPPORT, &hints, &ai)) != 0) {
		fprintf(stderr, "getaddrinfo tcp: %s\n", gai_strerror(rv));
		return 1;
	}



	// loop through all the results and make a socket
	for(res = ai; p != NULL; res = res->ai_next) {
		if ((tcpsockfd = socket(res->ai_family, res->ai_socktype,
				res->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

		break;
	}

	if (res == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}

  /* clear sets for select() */
  FD_ZERO(&readfds);
  FD_ZERO(&master);
  FD_ZERO(&writefds);
  /* Add udp socket to both write and read fds */
  FD_SET(sockfd, &master);
  FD_SET(tcpsockfd, &master);
  //FD_SET(sockfd, &master);
  if(tcpsockfd > sockfd){
    fdmax = tcpsockfd;
  }
  else {
    fdmax = sockfd;/* From beej's guide, TODO: UNDERSTAND */
  }

  printf("Input nick: ");
  scanf("%s", nick);

  /* send nick as join msg to server */
  //join(nick, sockfd, p->ai_addr, p->ai_addrlen);

  /* Try to TCP with the server straight away */
  if ((connect(tcpsockfd, res->ai_addr, res->ai_addrlen)) == -1){
    perror("Can't connect TCP!\n");
  }
  TCPjoin(tcpsockfd, nick);

  /* MAIN LOOP */
  while(1){

    readfds = master;
    writefds = master;
    memset(chatmsg, '\0', sizeof(chatmsg));

    if (select(fdmax+1, &readfds, &writefds, NULL, NULL) == -1){
      perror("select");
    }

    for(int i = 0; i <= fdmax; i++){
      if(FD_ISSET(i, &readfds)){
        if (i == sockfd){
          getinfo(i, p->ai_addr, p->ai_addrlen);
        }
        else if(i == tcpsockfd){
          if ((nbytes = recv(tcpsockfd, chatmsg, sizeof(buf), 0)) == -1){
            perror("revc tcp\n");
          }
          //chatmsg[nbytes] = "\n";
          printf("CHAT MSG: %s\n", chatmsg);
        }
      }
      else if(FD_ISSET(i, &writefds)){
        if (i == sockfd){
          /* player guessed right and can now set the new number to be guessed */
          if (won == 1){
            sendnumber(sockfd, p->ai_addr, p->ai_addrlen);
            won = 0;
          }
          else if(isMyTurn && won == 0){
            //printf("Your turn! state of won %d \n", won);
            sendguess(nick, sockfd, p->ai_addr, p->ai_addrlen);
            isMyTurn = 1;
          }
        }
        else if (i == tcpsockfd){
          Chat(tcpsockfd);
        }
      }
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
