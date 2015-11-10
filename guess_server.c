#include "guess_server.h"

#define MAXBUFLEN 1024
#define PMAX 10
#define TCPPORT "53100"

/* player data structure */
struct player{
	char nick[24];
	struct sockaddr address;
	socklen_t len;
	struct player *next;
	char buffer[MAXBUFLEN];
	int tcpsock;
};

/* Warning: GLOBAL VARIABLES! */
struct player *head = NULL, *end = NULL;
int number = -99; // number to guess
int isGuessed = 0; // has the number been guessed?
char debugmsg[MAXBUFLEN] = "======================================";


void addplayer(struct sockaddr *ip, char* nick, socklen_t len);
void addplayerTCP(int fd, struct sockaddr *ip, socklen_t len);

void testprint(struct player *head);
void unpackmsg(int socket,  struct sockaddr *from, char *msg, socklen_t len);


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
  return numb==-1?-1:0;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int server(char* port){

	int sockfd, listener;
	struct addrinfo hints, *servinfo, *p, *ai;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	char chatbuf[MAXBUFLEN];
	socklen_t addr_len;
	fd_set readfds, writefds, master;
	int fdmax;
	int newfd ,i, len;
	int yes = 1;
	char remoteIP[INET6_ADDRSTRLEN];

	//char s[INET6_ADDRSTRLEN];

	/* Initialization of TCP listener socket */
	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, TCPPORT, &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}

	for(p = ai; p != NULL; p = p->ai_next) {
    	listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) {
			continue;
		}

		// lose the pesky "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}

		break;
	}

	// if we got here, it means we didn't get bound
	if (p == NULL) {
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}

	freeaddrinfo(ai); // all done with this
	/* Listen for TCP connections */
	if(listen(listener, 10) == -1){
		perror("Listenin vika\n");
	}
	/* TCP LISTENER */


	/* Initialization of UDP socket */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);



	/* clear sets for select() */
  FD_ZERO(&readfds);
  FD_ZERO(&master);
  FD_ZERO(&writefds);
  /* Add udp and tcp sockets to both write and read fds */
  FD_SET(sockfd, &master);
	FD_SET(listener, &master);
  //FD_SET(sockfd, &master);

	/* set the biggest socket */
	if(listener > sockfd){
		fdmax = listener;/* From beej's guide, TODO: UNDERSTAND */
	}
	else{
		fdmax = sockfd;
	}


	char test[] = "TOIMII";
	char test2[] ="-------";
	/* MAIN LOOP  */
	while(1){
		readfds = master;
    writefds = master;
		//printf("%s\n", debugmsg);
		memset(buf, '\0', sizeof(buf));
		memset(chatbuf, '\0', sizeof(chatbuf));


		if (select(fdmax+1, &readfds, &writefds, NULL, NULL) == -1){
			perror("select");
		}

		/* loop through TCP connections */
		for(i = 0; i <= fdmax; i++){
			if(FD_ISSET(i, &readfds)){
				if(i == listener && i != sockfd){

					addr_len = sizeof their_addr;
					newfd = accept(listener, (struct sockaddr *)&their_addr, &addr_len);
					if(newfd == -1){
						perror("accept failed\n");
					}
					else{
						FD_SET(newfd, &master); /* add new socket to master set */
						if (newfd > fdmax){
							fdmax = newfd;
						}
						/* Add new player */
						addplayerTCP(newfd, (struct sockaddr *)&their_addr, addr_len);
						printf("Some one joined over TCP!\n");
						testprint(head);
					}
				}
				else if (i == sockfd){
					/* RECEIVE GUESSES */
					addr_len = sizeof their_addr;
					if ((numbytes = recvfrom(i, buf, MAXBUFLEN-1 , 0,
						(struct sockaddr *)&their_addr, &addr_len)) == -1) {
						perror("recvfrom");
						exit(1);
					}
					strcpy(test2, test);
					buf[numbytes] = '\0';
					printf("gotfrom client: %s\n", buf);

					unpackmsg(i, (struct sockaddr *)&their_addr, buf, addr_len);
				}
				else{ /*TCP Chat msg*/
					if ((numbytes = recv(i, chatbuf, sizeof chatbuf, 0)) <= 0){
						if (numbytes == 0){
							/* Connections closed */
							printf("%s\n", "A connections closed\n");
						}
						else{
							perror("revc in main loop DIED!\n");
						}
						close(i);
						FD_CLR(i, &master);
					}
					/* Send msg to all */
					for(int j = 0; j <= fdmax; j++){
						if (FD_ISSET(j, &master)){
							/* not to ourselves or udp sock */
							if (j != i && j != sockfd && j != listener){
								len = strlen(chatbuf);
								if ((sendAllTCP(j, chatbuf, &len)) == -1){
									perror("SENDALL");
								}
							}
						}
					}
				}
			}
		}
	}


	close(sockfd);

	return 0;

}

int main(int argc, char *argv[]){

	if(argc == 2){
		if(atoi(argv[1]) >= 1024 && atoi(argv[1]) < 65000){
			server(argv[1]);
		}
		else{
			printf("Port must be above 1023 and below 65000\n");
		}
	}
	else {
		printf("Usage: guessserver portnumber\n");
	}

}

/* adds a new player in the end of the linked list */
void addplayer(struct sockaddr *ip, char* nick, socklen_t len){
	struct player *tmp;


	tmp = (struct player*)malloc(sizeof(struct player));
	tmp->address = *ip;
	strcpy(tmp->nick, nick);
	tmp->len = len;
	tmp->next = NULL;

	/* First player to connect */
	if(head == NULL){
		printf("tmp nick: %s ja head on null \n", tmp->nick);
		head = tmp;
		end = tmp;
	}
	/* not the firs player, add to the end of the list */
	else{
		end->next = tmp;
		end = tmp;
	}
}

/*TEST ADD*/
/* adds a new player in the end of the linked list */
void addplayerTCP(int fd, struct sockaddr *ip, socklen_t len){
	struct player *tmp = head;
	char nickbuf[MAXBUFLEN];
	int nbytes;
	memset(nickbuf, '\0', sizeof(nickbuf));

	/* receive nick */
	if ((nbytes = recv(fd, nickbuf, sizeof(nickbuf), 0)) <= 0){
		/* error or connection closed */
		if (nbytes == 0){
			printf("%s\n", "A connection closed\n");
		}
		else{
			perror("recv in addplayertcp\n");
		}
	}

	tmp = (struct player*)malloc(sizeof(struct player));
	tmp->address = *ip;
	printf("In addp IP: %s\n", get_in_addr(ip));
	strcpy(tmp->nick, nickbuf);
	tmp->len = len;
	tmp->tcpsock = fd;
	tmp->next = NULL;

	/* First player to connect */
	if(head == NULL){
		printf("tmp nick: %s ja head on null \n", tmp->nick);
		head = tmp;
		end = tmp;
	}
	/* not the firs player, add to the end of the list */
	else{
		end->next = tmp;
		end = tmp;
	}
}
/*TEST ADD ENDS*/

void testprint(){
	struct player *tmp = head;
	while(tmp != NULL){
		printf("%s\n", tmp->nick);
		tmp = tmp->next;
	}
}

/* compare the ip adresses , returns 0 if they are the same */
int ipcmp(struct sockaddr *from, struct sockaddr *player){
	char s1[INET6_ADDRSTRLEN];
	char s2[INET6_ADDRSTRLEN];
	strcpy(s1, get_in_addr(from));
	strcpy(s2, get_in_addr(player));
	return strcmp(s1, s2);
}

/* Handle clients guesses */
void checkguess(struct sockaddr *from, char *msg, int socket, socklen_t len){
	struct player *tmp = head;
	char tmpmsg[MAXBUFLEN], brdcstmsg[MAXBUFLEN] = "-16 ";
	char nick[24];
	char *guess;

	strcpy(tmpmsg, msg);
	guess = strtok(tmpmsg, " "); // split on spaces "msgtype nick guess"
	guess = strtok(NULL, " ");   // split again to get nick
	strcpy(nick, guess);
	guess = strtok(NULL, " ");	// split again to get the guess

/* if IP and the nick is the same we can be sure it's the rigth person */
	printf("%s guessed %d\n",nick, atoi(guess));
	printf("guess is:%d\n", atoi(guess));
	/* See if it's the right number! */
	if(atoi(guess) == number){
		printf("%s\n", "And it was the right number!");
		isGuessed = 1;
		/* Ask for new number */
		askfornum(socket, from, len);

		/* let other players know what was guessed */
		strcat(brdcstmsg, tmp->nick);
		strcat(brdcstmsg, " ");
		strcat(brdcstmsg, " Guessed");
		strcat(brdcstmsg, " Right");
		strcpy(debugmsg, brdcstmsg);
		broadcastGuess(socket, brdcstmsg, strlen(brdcstmsg));
	}
	else {
		/* let other players know what was guessed */
		strcat(brdcstmsg, nick);
		strcat(brdcstmsg, " ");
		strcat(brdcstmsg, guess);
		strcpy(debugmsg, brdcstmsg);
		broadcastGuess(socket, brdcstmsg, strlen(brdcstmsg));
	}
}

/* unpack msg and act upon it's type */
void unpackmsg(int socket,  struct sockaddr *from, char *msg, socklen_t len){
	char tmpmsg[256];
	strcpy(tmpmsg, msg); // copy the original msg so that strtok wont affect it
	char *p; // for strtok
	p = strtok(tmpmsg, " ");
	if(atoi(p) == 1){
		printf("join msg\n");
		p = strtok(NULL, " "); /* Get nick from packet */
		addplayer(from, p, len);
	}
	/* it's a guess */
	else if(atoi(p) == 2){
		printf("number: %d\n", number);
		checkguess(from, msg, socket, len);


	}
	/* A new number to be guessed */
	else if(atoi(p) == 3){
		if(isGuessed) {
			setnumber(msg);
			isGuessed = 0;
		}
	}
}

/* Set the new number to be guessed*/
void setnumber(char *msg){
	char tmpmsg[256], *pch;
	strcpy(tmpmsg, msg);
	int newnum;
	/* strtok() twice to get the new message from the header/packet */
	pch = strtok(tmpmsg, " ");
	pch = strtok(NULL, " ");
	newnum = atoi(pch);
	printf("New number to be set: %d", newnum);
	number = newnum;
}

void askfornum(int socket, const struct sockaddr *to, socklen_t tolen){
	char msg[MAXBUFLEN] = "-99";
	if((sendto(socket, msg, strlen(msg), 0, to, tolen)) == -1){
		perror("asknum sendto");
	}
}

/* Inform player about other players guesses */
void broadcastGuess(int sockfd, char *msg, int len){
  struct player *tmp = head;
	int n;
	printf("viestihän on:%s:\n", msg);
	while(tmp != NULL){
		if((n=sendto(sockfd, msg, len, 0, &tmp->address, tmp->len)) == -1){
			perror("Broadcast sendto\n");
		}
		printf("brdcstSendto: %s IP:%s\n", tmp->nick,get_in_addr(&tmp->address));
		printf("Sent %d bytes\n", n);
		tmp = tmp->next;
	}

}
