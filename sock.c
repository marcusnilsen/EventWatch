/*
 *
 *                         THE WatchEvents PROGRAM                           *
 *                            2011 +-+ mables                                *
 *                      https://github.com/mables                            *
 *
 *
 *      gcc -o watchEvents watchEvents.c -Wall -W -Wextra -ansi
 *
 *
 *	Check for incoming message on server: nc -l -u -p 2000
 *
 */

#define DEBUG 0

#include "sock.h"
#include "watchEvents.h"

#include <string.h>		/* strlen */

/* std libs */
/*
#include <stdio.h>
#include <stdlib.h> 
*/

/* udp client lbs */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>

/* Define the local server to send the events to */
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 20000

int sendMessage(char *message) {
	#if DEBUG
		printf("Sending '%s' to server\n", message);
	#endif

	char mType[] = "Event:";
	char *fullMessage = (char *) malloc(sizeof(mType)+sizeof(message));

	int socketfp;
	struct sockaddr_in serv_addr;

	/* Setting up the message string to be sent (using strcat() causes a bug in fbsd) */
	sprintf(fullMessage, "%s%s", mType, message);

	/* set up a UDP socket for sending the message */	
	if((socketfp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		printf("Error opening socket.\n");
		return 1;
	}

	/* set up the internet addr of the event server */
	bzero((char *) &serv_addr, sizeof(serv_addr)); /* bzero is deprecated, use memset on new systems */
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERVER_PORT);
	serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	
	/* send the message to the event server */
	if(sendto(socketfp, fullMessage, strlen(fullMessage), 0, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in)) < 0) {
		printf("Error in sending message\n"); 
		return 1;
	}
	
	/* close the socket */
	free(fullMessage);
	close(socketfp);
	return 0;
}







