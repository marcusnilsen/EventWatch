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

/* Kqueue and file control */
#include <sys/types.h>		/* needed for kqueue on fbsd, but works without on OSX */
#include <sys/event.h> 		/* kqueue */
#include <sys/time.h> 		/* kqueue */
#include <fcntl.h>		/* file control */
#include <dirent.h>		/* dir traversing */
#include <unistd.h> 		/* to close */
#include <string.h>		/* strlen */
#include <sys/stat.h>		/* struct stat */

/* std libs */
#include <stdio.h>
#include <stdlib.h> 

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

	/* Setting up the message string to be sent */
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

void getLatestFile(char *dirname) {
	DIR *dirvar;
	struct dirent *result;
	struct stat statbuf;
	int newFileStamp = 0;
	char *newFileName = (char *) malloc(1);
	dirvar = opendir(dirname);

	while ((result=readdir(dirvar))!=NULL) {
		if(strcmp(".", result->d_name) < 0 || strcmp("..", result->d_name) < 0) {
			/* get full path for stat() */
			int len = strlen(dirname) + strlen(result->d_name) + 1;
			char *fullpath = malloc(len);
			strcpy(fullpath, dirname);
			strcat(fullpath, result->d_name);

			if (stat(fullpath, &statbuf) == -1) {
				printf("fsize: can't access %s\n", result->d_name);
				closedir(dirvar);
				return;
			}

			int tstamp = statbuf.st_mtime;

			if (tstamp >= newFileStamp) {
				newFileName = (char*) realloc (newFileName, sizeof(result->d_name));
				newFileStamp = tstamp;
				memcpy(newFileName, result->d_name, sizeof(result->d_name));
			}
			#if DEBUG
				printf(".");
			#endif
			free(fullpath);
		}
	}
	#if DEBUG
		printf("newest file is: %s\n", newFileName);
	#endif
	if(sendMessage(newFileName) == 0) {
		#if DEBUG
			printf("Message sent to server..\n");
		#endif
	} else {
		printf("Could not send message to server!\n");
	}
	free(result);
	free(newFileName);
	closedir(dirvar);
}


/* main */
int main (int argc, char *argv[]) {
	int f, kq, nev;
	struct kevent change;
	struct kevent event;
	if (argc != 2) {
		printf("Usage: %s directory\n", argv[0]);
		return 1;
	}

	kq = kqueue();
	if (kq == -1) {
		printf("Could not init kqueue.\n");	
		return 1;
	}

	f = open(argv[1], O_RDONLY);
	if (f == -1) {
		printf("Could not open directory.\n");
		return 1;
	}

	EV_SET(&change, f, EVFILT_VNODE, 
		EV_ADD | EV_ENABLE | EV_ONESHOT, 
		NOTE_DELETE | NOTE_EXTEND | NOTE_WRITE | NOTE_ATTRIB,
		0, NULL);

	for(;;) {
		nev = kevent(kq, &change, 1, &event, 1, NULL);
		if (nev == -1) {
			close(kq);
			close(f);
			printf("Could not init kevent.\n");
			return 1;
		} else if (nev > 0) {
			if (event.fflags & NOTE_DELETE) {
				#if DEBUG
					printf("File deleted\n");
				#endif
				break;
			}
			if (event.fflags & NOTE_EXTEND || event.fflags & NOTE_WRITE) {
				#if DEBUG
					printf("Dir modified, running check..");
				#endif
				getLatestFile(argv[1]);
			}
		}
	}
	
	close(kq);
	close(f);
	return 0;
}









