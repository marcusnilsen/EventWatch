/*
 *
 *                         THE WatchEvents PROGRAM                           *
 *                            2011 +-+ mables                                *
 *                      https://github.com/mables                            *
 *
 *
 *     gcc -o watchEvents watchEvents.c -Wall -W -Wextra -ansi -pedantic
 *
 *	Check for incoming message on server: nc -l -u -p 2000
 */

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
#define SERVER_PORT 2000

int sendMessage(char *message) {
	#if DEBUG
		printf("Sending '%s' to server\n", message);
	#endif

	int socketfp;
	struct sockaddr_in serv_addr;

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
	if(sendto(socketfp, message, strlen(message), 0, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in)) < 0) {
		printf("Error in sending message\n"); 
		return 1;
	}
	
	/* close the socket */
	close(socketfp);
	return EXIT_SUCCESS;
}

void getLatestFile(char *dirname) {
	DIR *dirvar;
	struct dirent *result;
	struct stat statbuf;
	int *newFileStamp = 0;
	char *newFileName = malloc(sizeof(*newFileName));
	dirvar = opendir(dirname);

	while ((result=readdir(dirvar))!=NULL) {
		if(strcmp(".", result->d_name) < 0 || strcmp("..", result->d_name) < 0) {
			/* get full path for stat() */
			int len = strlen(dirname) + strlen(result->d_name) + 1;
			char* fullpath = malloc(len);
			strcpy(fullpath, dirname);
			strcat(fullpath, result->d_name);

			if (stat(fullpath, &statbuf) == -1) {
				fprintf(stderr, "fsize: can't access %s\n", result->d_name);
				return;
			}

			int tstamp = statbuf.st_mtime;

			if (tstamp >= (int*) newFileStamp) {
				newFileStamp = (int*) malloc (tstamp+1);
				newFileName = (char*) malloc (*result->d_name+1);

				newFileStamp = (int) tstamp;
				newFileName = result->d_name;
			}
			#if DEBUG
				printf(".");
			#endif
		}
	}
	#if DEBUG
		printf("newest file is: %s\n", newFileName);
	#endif
	if(sendMessage(newFileName)) {
		#if DEBUG
			printf("Message sent to server..\n");
		#endif
	} else {
		printf("Could not send message to server!\n");
	}
	free(result);
	closedir(dirvar);
}


/* main */
int main (int argc, char *argv[]) {
	int f, kq, nev;
	struct kevent change;
	struct kevent event;
	if (argc != 2) {
		fprintf(stderr, "Usage: %s directory\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	kq = kqueue();
	if (kq == -1) {
		perror("kqueue");
	}

	f = open(argv[1], O_RDONLY);
	if (f == -1) {
		perror("open");
	}

	EV_SET(&change, f, EVFILT_VNODE, 
		EV_ADD | EV_ENABLE | EV_ONESHOT, 
		NOTE_DELETE | NOTE_EXTEND | NOTE_WRITE | NOTE_ATTRIB,
		0, NULL);

	for(;;) {
		nev = kevent(kq, &change, 1, &event, 1, NULL);
		if (nev == -1) {
			perror("kevent");
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
	return EXIT_SUCCESS;
}









