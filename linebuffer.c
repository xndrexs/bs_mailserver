#include "linebuffer.h"
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <string.h>

/*
typedef struct lbuf {
    int descriptor;             	Eingabe-Descriptor
    const char *linesep;        	Zeilentrenner, C-String
    unsigned lineseplen;        	Länge des Zeilentrenners in Bytes
    char buffer[LINEBUFFERSIZE];	Lesebuffer
    unsigned bytesread;         	Anzahl bereits von descriptor gelesener Bytes 
    unsigned here;              	aktuelle Verarbeitungsposition im Lesebuffer 
    unsigned end;               	Anzahl belegter Bytes im Lesebuffer
} LineBuffer;
*/

LineBuffer *buf_new(int descriptor, const char *linesep) {
	LineBuffer *buf = calloc(1, sizeof(LineBuffer));
	buf -> descriptor = descriptor;
	buf -> linesep = linesep;
	buf -> lineseplen = strlen(linesep);
	buf -> bytesread = 0;
	buf -> here = 0;
	return buf;
}

void buf_dispose(LineBuffer *b) {
	free(b);
}

int read_buffer(LineBuffer *b) {
	
	int fd_read = 0;
	
	if((fd_read = read(b -> descriptor, b -> buffer, LINEBUFFERSIZE)) < 0) {
		perror("read, read_line");
	}
	
	b -> bytesread = b -> bytesread + fd_read;
	b -> end = fd_read;
	b -> here = 0;
	/*
	printf("Reading buffer ... \n");
	printf("... bytes read: %u\n", fd_read);
	printf("Total Bytes read: %d\n", b -> bytesread);
	*/
	return fd_read;
}


int buf_readline(LineBuffer *b, char *line, int linemax) {
	
	char *current_pos = &(b -> buffer[b -> here]);
	int line_cur = 0;
	/*char *display_line = line;*/
	
	
	/*printf("----------------------------\n");*/
	
	
	if (b -> bytesread == 0) {
		read_buffer(b);
	}
	
	/*
	printf("Buffer here: %u\n", b -> here);
	printf("Buffer end: %u\n", b -> end);
	*/
	
	while(1) {
		/* Zeilentrenner erreicht */
		if (*current_pos == *(b -> linesep)){
			b -> here++;
			/*printf("LINE BREAK\n");*/
			break;
		/* Zeichen in Line kopieren */
		/*} else if (b -> here == linemax){
			printf("LINE MAX\n");
			break;*/
			/* Buffer-Ende erreicht */
		} else if(b -> here >= b -> end) {
			/*printf("End of Buffer reached: %d / %d\n", b -> here, b -> end);*/
			read_buffer(b);
			current_pos = &(b -> buffer[b -> here]);
			/* Nichts mehr einzulesen */
			if (b -> end == 0) {
				/*
				printf("TOTAL BYTES READ: %d\n", b -> bytesread);
				printf("EOF\n");
				*/
				return -1;
			}
		} else {
			*line = *current_pos;
			line++;
			current_pos++;
		}
		

		b -> here++;
		
		line_cur++;
	}
	*line = '\0';
	
	/*printf("Buffer new here: %d\n", b -> here);*/
	
	return (b -> bytesread) + (b -> here);
}

int buf_where(LineBuffer *b) {
	return (b -> bytesread - b -> end + b -> here);
}

int buf_seek(LineBuffer *b, int seekpos) {
	int pos = lseek(b -> descriptor, seekpos, SEEK_SET);
	b -> bytesread = pos;
	b -> here = 0;
	b -> end = 0;
	return pos;
}
