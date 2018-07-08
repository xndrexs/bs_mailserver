#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#include "dialog.h"
#include "database.h"
#include "linebuffer.h"
#include "fileindex.h"
#include "config.h"

const char smtp_ready[] = "220 Ein ganz toller SMTP-Server.\r\n";
const char error[] = "500\n";
const char smtp_ok[] = "250 Ok\r\n";
const char smtp_quit[] = "221 Bye bye.\r\n";
const char start_data[] = "354 Data now.\r\n";
int client_socket = 0;
char *path_to_mb;

int get_filesize(const char *path);
int my_printf(const char *message);
int validate_noparam(DialogRec *d);
int helo(DialogRec *d);
int mail_from(DialogRec *d);
int rcpt_to(DialogRec *d);
int data(DialogRec *d);
int quit_smtp(DialogRec *d);
int process_lock(const char *filepath);
int remove_lock_file(const char *path);

DialogRec smtp_dialogs [] = {
	/*command (17)		param (80)	state	nextstate	validator */
	{ "helo",			"", 		0, 		1,	helo				},
	{ "mail from:",		"",			1,		2,	mail_from			},
	{ "rcpt to:",		"",			2, 		3,	rcpt_to				},
	{ "data",			"", 		3,		4, 	data				},
	{ "quit",			"", 		4,		0,	quit_smtp			},
	{ ""															}
};

/* E-Mail bauen und abspeichern */
int build_email(){
	
	char out[100];
	int fd_open = 0, fd_write = 0, fd_read = 0;
	int file_size = 0;
	char *content;
	time_t ltime = 0;
	time(&ltime);
	
	/* Zwischen gespeicherte Mail (nur Inhalt) öffnen */
	if((fd_open = open(mailbox_tmp, O_RDONLY)) < 0){
		perror("error open");
	}
	
	/* Inhalt einlesen */
	file_size = get_filesize(mailbox_tmp);
	content = malloc(file_size);
	if ((fd_read = read(fd_open, content, file_size)) < 0){
		perror("read");
	}
	
	close(fd_open);
	
	/* Mailbox öffnen */
	if((fd_open = open(path_to_mb, O_RDWR | O_APPEND)) < 0) {
		perror("error open");
	}
	
	/* From Zeile generieren und schreiben */
	sprintf(out, "From %s %s\n", smtp_dialogs[1].param, ctime(&ltime));
	
	if ((fd_write = write(fd_open, out, strlen(out))) < 0){
		perror("error write");
	}
	
	/* Inhalt darunter schreiben */
	if ((fd_write = write(fd_open, content, file_size)) < 0){
		perror("error write");
	}
	
	close(fd_open);
	remove(mailbox_tmp);
	remove_lock_file(path_to_mb);
	return 0;
}

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
	char *from;
	if (result != 0) {
		from = smtp_dialogs[1].param;
		from[strlen(from)-1] = 0;
		write(client_socket, smtp_ok, strlen(smtp_ok));
		result = 1;
	}
	return result;
}

/* rcpt to */
int rcpt_to(DialogRec *d){
	int result = validate_noparam(d);
	char *from;
	DBRecord *record;
	if (result != 0) {
		from = smtp_dialogs[2].param;
		from[strlen(from)-1] = 0;
		record = malloc(sizeof(DBRecord));
		strcpy(record -> key, smtp_dialogs[2].param);
		strcpy(record -> cat, cat_smtp);
		/* Prüfen, ob Empfänger vorhanden ist */
		result = db_search(database, 0, record);
		if (result >= 0) {
			strcpy(record -> key, record -> value);
			strcpy(record -> cat, "mailbox");
			/* Mailbox zum Emfänger finden */
			result = db_search(database, 0, record);
			if (result >= 0) {
				result = 1;
				path_to_mb = malloc(DB_VALLEN); 
				strcpy(path_to_mb, record -> value); /* Mailbox merken */
				write(client_socket, smtp_ok, strlen(smtp_ok));
				process_lock(path_to_mb);
			}
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
	LineBuffer *b = buf_new(client_socket, "\r\n");
	
	if (result == 0) {
		fd_open = open(mailbox_tmp, O_RDWR | O_CREAT | O_TRUNC, 0644);
		if (fd_open < 0){
			perror("error read");
			exit(1);
		}
		write(client_socket, start_data, strlen(start_data));
		while(1){
			
			line = calloc(1, linemax);
			buf_readline(b, line, linemax);
			if (strcmp(line, ".") == 0){
				free(line);
				fd_write = write(fd_open, "\n", strlen("\n"));
				if (fd_write < 0){
					perror("error write");
					exit(1);
				}
				break;
			}
			fd_write = write(fd_open, line, strlen(line));
			if (fd_write < 0){
				perror("error write");
				exit(1);
			}
			fd_write = write(fd_open, "\n", strlen("\n"));
			if (fd_write < 0){
				perror("error write");
				exit(1);
			}
			free(line);
			
		}
		close(fd_open);
		close(fd_write);
		write(client_socket, smtp_ok, strlen(smtp_ok));
		result = 1;
		build_email();
		
	}
	free(b);
	return result;
}

/* quit */
int quit_smtp(DialogRec *d){
	int result = validate_noparam(d);
	if (result == 0){
		write(client_socket, smtp_quit, strlen(smtp_quit));
		close(client_socket);
		return -17;
	}
	return 1;
}

void *process_smtp(void *args) {
	
	ProlResult prolresult;
	int linemax = 1024;	
	int state = 0;
	char *line;
	LineBuffer *b;
	
	client_socket = *((int*)args);
	b = buf_new(client_socket, "\r\n");
	
	write(client_socket, smtp_ready, strlen(smtp_ready));
	
	while(1){
		line = calloc(1, linemax);
		if(buf_readline(b, line, linemax) < 0) {
			break;
		};
		prolresult = processLine(line, state, smtp_dialogs);
		free(line);
		
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
	free(b);
	return NULL;
}
