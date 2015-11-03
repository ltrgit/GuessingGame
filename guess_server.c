#include "guess_server.h"

#define MAXBUFLEN 1024
#define PMAX 10

/* player data structure */
struct player{
	char nick[24];
	struct sockaddr address;
	socklen_t len;
	struct player *next;
};

/* Warning: GLOBAL VARIABLES! */
struct player *head = NULL, *end = NULL;
int number = -99; // number to guess
int isGuessed = 0; // has the number been guessed?
char debugmsg[MAXBUFLEN] = "======================================";

void addplayer(struct sockaddr *ip, char* nick, socklen_t len);
void testprint(struct player *head);
void unpackmsg(int socket,  struct sockaddr *from, char *msg, socklen_t len);

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int server(char* port){

	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	fd_set readfds, writefds, master;
	int fdmax;

	//char s[INET6_ADDRSTRLEN];



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
	/* players */
	//struct player *head = NULL, *end = NULL;
	//head = malloc(sizeof(struct player));
	/*if(head == NULL){
		printf("voe vittu\n");
		exit(1);
	}*/

	/* clear sets for select() */
  FD_ZERO(&readfds);
  FD_ZERO(&master);
  FD_ZERO(&writefds);
  /* Add udp socket to both write and read fds */
  FD_SET(sockfd, &master);
  //FD_SET(sockfd, &master);
  fdmax = sockfd;/* From beej's guide, TODO: UNDERSTAND */

	char test[] = "TOIMII";
	char test2[] ="-------";
	/* MAIN LOOP  */
	while(1){
		readfds = master;
    writefds = master;
		//printf("%s\n", debugmsg);


		if (select(fdmax+1, &readfds, &writefds, NULL, NULL) == -1){
			perror("select");
		}

		/* Check for any data to receive */
		/* TODO TCP CHAT */
		if (FD_ISSET(sockfd, &readfds)){
			/* RECEIVE GUESSES */
			addr_len = sizeof their_addr;
			if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
				(struct sockaddr *)&their_addr, &addr_len)) == -1) {
				perror("recvfrom");
				exit(1);
			}
			strcpy(test2, test);
			buf[numbytes] = '\0';

			unpackmsg(sockfd, (struct sockaddr *)&their_addr, buf, addr_len);
		}


		/* INFORM OTHER PLAYERS ABOUT GUESSES */
		/*char te[] = "-99";
		if(sendto(sockfd, te , strlen(te), 0, (struct sockaddr *)&their_addr, addr_len) == -1){
			perror("server send error\n");
			exit(1);
		}*/


		/*printf("listener: got packet from %s\n",
			inet_ntop(their_addr.ss_family,
				get_in_addr((struct sockaddr *)&their_addr),
				s, sizeof s));
		//printf("listener: packet is %d bytes long\n", numbytes);*/
		//buf[numbytes] = '\0';
		//printf("listener: packet contains \"%s\"\n", buf);

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

	printf("ORIG MSG: %s\n", tmpmsg);
	guess = strtok(tmpmsg, " "); // split on spaces "msgtype nick guess"
	guess = strtok(NULL, " ");   // split again to get nick
	strcpy(nick, guess);
	guess = strtok(NULL, " ");	// split again to get the guess

	while(tmp != NULL){
		if(ipcmp(&tmp->address, from) == 0){
			/* if IP and the nick is the same we can be sure it's the rigth person */
			if(strcmp(tmp->nick, nick) == 0){
				printf("%s guessed %d\n",tmp->nick, atoi(guess));

				/* See if it's the right number! */
				if(atoi(guess) == number){
					printf("%s\n", "And it was the right number!");
					isGuessed = 1;
					/* Ask for new number */
					askfornum(socket, from, len);
				}
				/* let other players know what was guessed */
				strcat(brdcstmsg, tmp->nick);
				strcat(brdcstmsg, " ");
				strcat(brdcstmsg, guess);
				strcpy(debugmsg, brdcstmsg);
				broadcastGuess(socket, brdcstmsg, strlen(brdcstmsg));
			}
		}
		tmp = tmp->next;
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
		printf("its a guess msg: %s\n", msg);
		checkguess(from, msg, socket, len);


	}
	/* A new number to be guessed */
	else if(atoi(p) == 3){
		if(isGuessed) {
			setnumber(&number,msg);
		}
	}
}

/* Set the new number to be guessed*/
void setnumber(int *numguess, char *msg){
	char tmpmsg[256], *pch;
	strcpy(tmpmsg, msg);
	int newnum;
	/* strtok() twice to get the new message from the header/packet */
	pch = strtok(tmpmsg, " ");
	pch = strtok(tmpmsg, " ");
	newnum = atoi(pch);
	*numguess = newnum;
}

void askfornum(int socket, const struct sockaddr *to, socklen_t tolen){
	char msg[MAXBUFLEN] = "-99";
	sendto(socket, msg, strlen(msg), 0, to, tolen);
}

/* Inform player about other players guesses */
void broadcastGuess(int sockfd, const void *msg, int len){
  struct player *tmp = head;
	while(tmp != NULL){
		sendto(sockfd, msg, len, 0, &tmp->address, tmp->len);
		tmp = tmp->next;
	}

}
