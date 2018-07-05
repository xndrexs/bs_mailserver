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
	b -> bytesread += fd_read;
	b -> end = fd_read;
	b -> here = 0;
	return fd_read;
}


int buf_readline(LineBuffer *b, char *line, int linemax) {
	
	/* Zeiger für aktuellen Buchstaben und Seperator */
	char *current_letter = &(b -> buffer[b -> here]);
	const char *current_sep = b -> linesep;
	
	/* Position des Zeilenanfangs merken */
	int offset = buf_where(b);
	int linecur = 0;
	
	/* 1. Mal einlesen */
	if (b -> bytesread == 0) {
		read_buffer(b);
	}
	
	while(1) {
		
		/* Ende des Buffers erreicht */
		if(b -> here >= b -> end) {
			read_buffer(b);
			current_letter = &(b -> buffer[b -> here]);
			/* Nichts mehr einzulesen */
			if (b -> end == 0) {
				return -1;
			}
		}
		/* Anfang des Zeilentrenners erreicht */
		if(*current_letter == *current_sep) {
			b -> here++;
			current_letter++;
			if(b -> lineseplen == 1){
				break;
			}
			current_sep++;

			if(*current_letter == *current_sep) {
				b -> here++;
				current_letter++;
				break;
			}
		}
		/* Zeichen kopieren, Zeiger vorwärts */
		*line++ = *current_letter++;
		b -> here++;
		if(linecur++ > linemax) {
			break;
		}
	}
	*line = '\0';
	return offset;
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
