#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "dialog.h"
#include "database.h"
#include "linebuffer.h"
#include "fileindex.h"

const char smtp_ready[] = "220 Ein ganz toller SMTP-Server.\r\n";
const char error[] = "500\n";
const char smtp_ok[] = "250 Ok\r\n";
const char smtp_quit[] = "221 Bye bye.\r\n";
const char start_data[] = "354 Data now.\r\n";
int client_socket;

const char *test = "/home/andreas/semester6/betriebssysteme/bs_mailserver/mailbox/test.mbox";

int my_printf(const char *message);
int validate_noparam(DialogRec *d);
int helo(DialogRec *d);
int mail_from(DialogRec *d);
int rcpt_to(DialogRec *d);
int data(DialogRec *d);
int quit_smtp(DialogRec *d);

DialogRec smtp_dialogs [] = {
	/*command (17)		param (80)	state	nextstate	validator */
	{ "helo",			"", 		0, 		1,	helo				},
	{ "mail from:",		"",			1,		2,	mail_from			},
	{ "rcpt to:",		"",			3, 		4,	rcpt_to				},
	{ "data",			"", 		5,		6, 	data				},
	{ "quit",			"", 		6,		0,	quit_smtp			},
	{ ""															}
};

/* helo */
int helo(DialogRec *d){
	int result = validate_noparam(d);
	if (result != 0) {
		result = write(client_socket, smtp_ok, strlen(smtp_ok));
	}
	return result;
}

/* mail from */
int mail_from(DialogRec *d){
	int result = validate_noparam(d);
	if (result != 0) {
		result = write(client_socket, smtp_ok, strlen(smtp_ok));
	}
	return result;
}

/* rcpt to */
int rcpt_to(DialogRec *d){
	int result = validate_noparam(d);
	if (result != 0) {
		result = write(client_socket, smtp_ok, strlen(smtp_ok));
	}
	return result;
}

/* data */
int data(DialogRec *d){
	int result = validate_noparam(d);
	int linemax = 1024;
	char *line;
	LineBuffer *b;
	
	if (result != 0) {
		result = write(client_socket, start_data, strlen(start_data));
		while(1){
			line = calloc(1, linemax);
			b = buf_new(client_socket, "\r");
			buf_readline(b, line, linemax);
			free(line);
			free(b);
		}
	}
	return result;
}

/* quit */
int quit_smtp(DialogRec *d){
	write(client_socket, smtp_quit, strlen(smtp_quit));
	close(client_socket);
	return validate_noparam(d);
}


void *process_smtp(void *args) {
	
	ProlResult prolresult;
	int linemax = 1024;	
	int state = 0;
	char *line;
	LineBuffer *b;
	client_socket = *((int*)args);
	write(client_socket, smtp_ready, strlen(smtp_ready));
	
	while(1){
		line = calloc(1, linemax);
		b = buf_new(client_socket, "\r");
		buf_readline(b, line, linemax);
		prolresult = processLine(line, state, smtp_dialogs);
		free(line);
		free(b);
		/* quit */
		if(prolresult.failed == -17){
			my_printf("Client disconnected");
			break;
		}
		
		if(prolresult.failed == 0){
			state = prolresult.dialogrec -> nextstate;
		} else {
			write(client_socket, error, strlen(error));
		}
	}
	return NULL;
}
