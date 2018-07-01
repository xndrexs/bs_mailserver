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
const char *smtp_cat = "smtp";
int client_socket;

const char *test = "/home/andreas/semester6/betriebssysteme/bs_mailserver/mailbox/test.mbox";
extern char *path;
char *path_to_mb;

int my_printf(const char *message);
int validate_noparam(DialogRec *d);
int helo(DialogRec *d);
int mail_from(DialogRec *d);
int rcpt_to(DialogRec *d);
int data(DialogRec *d);
int quit_smtp(DialogRec *d);

DialogRec smtp_dialogs [] = {
	/*command (17)		param (80)	state	nextstate	validator */
	{ "helo",			"", 		1, 		1,	helo				},
	{ "mail from:",		"",			1,		2,	mail_from			},
	{ "rcpt to:",		"",			2, 		3,	rcpt_to				},
	{ "data",			"", 		0,		4, 	data				},
	{ "quit",			"", 		4,		0,	quit_smtp			},
	{ ""															}
};

/* helo */
int helo(DialogRec *d){
	int result = validate_noparam(d);
	if (result != 0) {
		write(client_socket, smtp_ok, strlen(smtp_ok));
		result = 1;
	}
	return result;
}

/* mail from */
int mail_from(DialogRec *d){
	int result = validate_noparam(d);
	if (result != 0) {
		write(client_socket, smtp_ok, strlen(smtp_ok));
		result = 1;
	}
	return result;
}

/* rcpt to */
int rcpt_to(DialogRec *d){
	int result = validate_noparam(d);
	DBRecord *record;
	if (result != 0) {
		
		record = malloc(sizeof(DBRecord));
		strcpy(record -> key, smtp_dialogs[2].param);
		strcpy(record -> cat, smtp_cat);
		/* Prüfen, ob Empfänger vorhanden ist */
		result = db_search(path, 0, record);
		if (result >= 0) {
			result = 1;
			path_to_mb = malloc(DB_VALLEN);
			strcpy(path_to_mb, record -> value);
			write(client_socket, smtp_ok, strlen(smtp_ok));
		} else {
			result = 0;
		}
		free(record);
		/* Ist vorhanden, wenn der Index von db_search >= 0 ist, 
		* muss aber etwas größer 0 zurückgeben, damit der Validator 
		* nicht meckert bzw. der Empfänger vorhanden ist
		* */
	}
	return result;
}

/* data */
int data(DialogRec *d){
	int result = validate_noparam(d);
	int linemax = 1024, fd_open, fd_write;
	char *line;
	LineBuffer *b;
	
	if (result == 0) {
		fd_open = open(test, O_RDWR | O_CREAT | O_TRUNC, 0644);
		if (fd_open < 0){
			perror("error read");
			exit(1);
		}
		write(client_socket, start_data, strlen(start_data));
		while(1){
			line = calloc(1, linemax);
			b = buf_new(client_socket, "\r");
			buf_readline(b, line, linemax);
			if (strcmp(line, ".") == 0){
				free(line);
				free(b);
				break;
			}
			fd_write = write(fd_open, line, strlen(line));
			if (fd_write < 0){
				perror("error write");
				exit(1);
			}
			fd_write = write(fd_open, "\r\n", strlen("\r\n"));
			if (fd_write < 0){
				perror("error write");
				exit(1);
			}
			free(line);
			free(b);
		}
		close(fd_open);
		close(fd_write);
		write(client_socket, smtp_ok, strlen(smtp_ok));
		result = 1;
	}
	return result;
}

/* quit */
int quit_smtp(DialogRec *d){
	write(client_socket, smtp_quit, strlen(smtp_quit));
	close(client_socket);
	return validate_noparam(d);
}

int build_email(){
	return 0;
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
