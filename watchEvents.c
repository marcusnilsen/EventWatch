/*
 *
 *                         THE WatchEvents PROGRAM                           *
 *                            2011 +-+ mables                                *
 *                      https://github.com/mables                            *
 *
 *
 *     gcc -o watchEvents watchEvents.c -Wall -W -Wextra -ansi -pedantic
 *
 */

/* libs */
#include <sys/event.h> 		/* kqueue */
#include <sys/time.h> 		/* kqueue */
#include <fcntl.h>		/* file control */
#include <dirent.h>		/* dir traversing */
#include <unistd.h> 		/* to close */
#include <string.h>		/* strlen */
#include <sys/stat.h>		/* struct stat */
#include <stdio.h>
#include <stdlib.h> 


#include <time.h>
#include <locale.h>
#include <langinfo.h>


void getLatestFile(char *dirname) {
	DIR *pdir;
	struct dirent *pent;
	struct stat statbuf;
	int *newFileStamp = 0;
	char *newFileName = malloc(sizeof(*newFileName));
	/* datestrings: */
	struct tm      *tm;
	char            datestring[256];
		
	pdir = opendir(dirname);

	while ((pent=readdir(pdir))!=NULL) {
		if(strcmp(".", pent->d_name) < 0 || strcmp("..", pent->d_name) < 0) {
			/* get full path for stat() */
			int len = strlen(dirname) + strlen(pent->d_name) + 1;
			char* fullpath = malloc(len);
			strcpy(fullpath, dirname);
			strcat(fullpath, pent->d_name);

			if (stat(fullpath, &statbuf) == -1) {
				fprintf(stderr, "fsize: can't access %s\n", pent->d_name);
				return;
			}

			tm = localtime(&statbuf.st_mtime);
			/* Get localized date string. */
 			strftime(datestring, sizeof(datestring), nl_langinfo(D_T_FMT), tm); 
			/* printf(" %s %s\n", datestring, pent->d_name); */

			int tstamp = statbuf.st_mtime;

			if (tstamp >= newFileStamp) {
				newFileStamp = (int*) malloc (tstamp+1);
				newFileName = (char*) malloc (*pent->d_name+1);

				newFileStamp = tstamp;
				newFileName = pent->d_name;
			}
			printf(".");
		}
	}
	printf("newest file is: %s\n", newFileName);
	free(pent);
	closedir(pdir);

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
				printf("File deleted\n");
				break;
			}
			if (event.fflags & NOTE_EXTEND || event.fflags & NOTE_WRITE) {
				printf("Dir modified, running check..");
				getLatestFile(argv[1]);
			}
		}
	}
	
	close(kq);
	close(f);
	return EXIT_SUCCESS;
}









