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

/* #include "sock.h" */
#include "watchEvents.h"


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
			char *fullpath = malloc(strlen(dirname) + strlen(result->d_name) + 1);
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

	/* Check if newest file is within last 5sec? to avoid announcing old files when a file is deleted */
	/* if(tstamp > now()-5sec) goto sendMessage: */
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
	int folder, kque, nev;
	struct kevent change;
	struct kevent event;
	if (argc != 2) {
		printf("Usage: %s directory\n", argv[0]);
		return 1;
	}

	kque = kqueue();
	if (kque == -1) {
		printf("Could not init kqueue.\n");	
		return 1;
	}

	folder = open(argv[1], O_RDONLY);
	if (folder == -1) {
		printf("Could not open directory.\n");
		return 1;
	}

	EV_SET(&change, folder, EVFILT_VNODE, 
		EV_ADD | EV_ENABLE | EV_ONESHOT, 
		NOTE_DELETE | NOTE_EXTEND | NOTE_WRITE | NOTE_ATTRIB,
		0, NULL);

	/* Main-loop */
	for(;;) {
		nev = kevent(kque, &change, 1, &event, 1, NULL);
		if (nev == -1) {
			close(kque);
			close(folder);
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
	
	close(kque);
	close(folder);
	return 0;
}









