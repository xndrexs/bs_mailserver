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

const char ok[] = "+OK\r\n";
const char err[] = "-ERR\r\n";
const char pop3_ready[] = "+OK Ein ganz toller Mailserver.\r\n";
const char logged_in[] = "+OK Logged in.\r\n";
/*const char *path = "/home/mi/apoeh001/semester6/betriebssysteme/mailserver/database/database";*/
const char *path = "/home/andreas/semester6/betriebssysteme/bs_mailserver/database/database";
const char cat_mailbox[] = "mailbox";
const char cat_password[] = "password";
const char seperator[] = "From ";
const char crlf[] = "\r\n";

int validate_onoff(DialogRec *d);
int validate_noparam(DialogRec *d);
int authenticate(DialogRec *d);
int show_mailbox_stats(DialogRec *d);
int show_message_details(DialogRec *d);
int show_message(DialogRec *d);
int no_operation(DialogRec *d);
int mark_for_dele(DialogRec *d);
int reset_marked(DialogRec *d);
int quit_connection(DialogRec *d);
int get_user(DialogRec *d);
int kill(pid_t pid, int sig);
int my_printf(const char *message);

FileIndex *fi;
int out;
int in;

DialogRec dialogs [] = {
	/*command (17)		param (80)	state	nextstate	validator */
	{ "user",			"", 		0, 		1,			get_user		 		},
	{ "pass",			"",			1,		2,			authenticate			},
	{ "stat",			"",			2, 		2,			show_mailbox_stats		},
	{ "list",			"", 		2,		2,			show_message_details	},
	{ "retr",			"", 		2,		2,			show_message			},
	{ "noop",			"",			2,		2, 			no_operation			},
	{ "dele",			"",			2,		2,			mark_for_dele			},
	{ "rset",			"",			2,		2,			reset_marked			},
	{ "quit",			"", 		2,		0,			quit_connection			},
	{ ""																		}
};

/* Lock File mit übergebenem Pfad löschen */
int remove_lock_file(const char *path) {
	char lock_path[1000];	
	char *lock_suffix = ".lock";	
	char *remove_path;
	remove_path = strcat(strcpy(lock_path, path), lock_suffix);
	return remove(remove_path);
}

/* Signal Handler für SIGINT) */
void handle_signal(int sig) {
	my_printf("Process interrupted");
	if (fi) {
		my_printf("Cleaning up ...");
		remove_lock_file(fi -> filepath);
		fi_dispose(fi);
	}
	exit(0);
}


int validate_noparam(DialogRec *d) {
	return strcmp(d->param, "");
}

int validate_onoff(DialogRec *d) {
	return !strcmp(d->param, "on") || !strcmp(d->param, "off");
}

/* noop */
int no_operation(DialogRec *d) {
	int result = validate_noparam(d);
	if (result == 0){
		write(out, ok, sizeof(ok));
	}
	return !result;
}

/* dele n */
int mark_for_dele(DialogRec *d) {
	FileIndexEntry *fie;
	int result = validate_noparam(d);
	int index;
	
	if (result != 0) {
		index = atoi(dialogs[6].param);
		if ((fie = fi_find(fi, index)) != NULL) {
			fie -> del_flag = 1;
			write(out, ok, sizeof(ok));
			result = 1;;
		} else {
			result = 0;
		}
	}
	return result;
}

/* rset */
int reset_marked(DialogRec *d) {
	FileIndexEntry *fie;
	if(validate_noparam(d) == 0) {
		fie = (fi -> entries);
		while(fie) {
			fie -> del_flag = 0;
			fie = fie -> next;
		}
		write(out, ok, sizeof(ok));
		return 1;
	}
	return 0;

}

/* quit */
int quit_connection(DialogRec *d) {
	int result = validate_noparam(d);
	char *logging_out = "+OK Bye bye.\r\n";

	if (result == 0){
		write(out, logging_out, strlen(logging_out));
		if (remove_lock_file(fi -> filepath) != 0) {
			perror("error deleting");
		}
		fi_compactify(fi);
		fi_dispose(fi);
		return -17;
	}
	return !result;;
}

/* stat */
int show_mailbox_stats(DialogRec *d){
	int size = 0;
	int entries = 0;
	FileIndexEntry *fie;
	char buffer[50] = "\0";
	if(validate_noparam(d) == 0) {
		fie = (fi -> entries);
		while(fie) {
			if(fie -> del_flag == 0) {
				size += fie -> size;
				entries++;
			}
			fie = fie -> next;
		}
		sprintf(buffer, "%s %d %d %s", "+OK", entries, size, crlf);
		write(out, buffer, strlen(buffer));
		return 1;
	} else {
		return 0;
	}
}

/* retr n */
int show_message(DialogRec *d) {
	FileIndexEntry *fie;
	LineBuffer *buf;
	int index;
	int lines = 0;
	int result = validate_noparam(d);
	int fd_open = 0;
	int linemax = 1024;
	char *line = malloc(linemax);
	char buffer[50] = "\0";

	if (result != 0) {
		if ((fd_open = open(fi -> filepath, O_RDONLY)) < 0) {
			perror("Error Open");
		}
		index = atoi(dialogs[4].param);
		fie = fi_find(fi, index);
		if (fie != NULL && fie -> del_flag == 0) {
			sprintf(buffer, "%s %d %s%s", "+OK", fie -> size, "octets", crlf);
			write(out, buffer, strlen(buffer));
			buf = buf_new(fd_open, "\n");
			buf_seek(buf, fie -> seekpos);
			while(lines < fie -> lines){
				buf_readline(buf, line, linemax);
				sprintf(buffer, "%s%s", line, crlf);
				write(out, buffer, strlen(buffer));
				lines++;
			}
			result = 1;
			write(out, ".\r\n", strlen(".\r\n"));
		} else {
			result = 0;
		}
	}
	free(line);
	return result;
}

/* list */
int show_message_details(DialogRec *d) {
	FileIndexEntry *fie = fi -> entries;
	int index;
	char buffer[80] = "\0";
	/* ohne Parameter */
	if(strcmp(dialogs[3].param, "") == 0){
		sprintf(buffer, "%s %d %s", "+OK", fi -> nEntries, crlf);
		write(out, buffer, strlen(buffer));
		while(fie) {
			if (fie -> del_flag == 0){
				sprintf(buffer, "%d %d %s", fie -> nr, fie -> size, crlf);
				write(out, buffer, strlen(buffer));
			}
			fie = fie -> next;
		}
		write(out, ".\r\n", strlen(".\r\n"));
	/* mit Parameter */
	} else {
		index = atoi(dialogs[3].param);
		fie = fi_find(fi, index);
		if (fie != NULL && fie -> del_flag == 0) {
			sprintf(buffer, "%s %d %d %s", "+OK", fie -> nr, fie -> size, crlf);
			write(out, buffer, sizeof(buffer));
		} else {
			return 0;
		}
	}

	return 1;
}

/* lock */
int process_lock(const char *filepath) {

	int fd_lock = 0;
	char lock_path[100];	
	char *lock_suffix = ".lock";
	char pid_buffer[10];
	char *lock;
	int status = 0, result = 0, pid = 0;
	char *locked_message;
	
	lock = strcat(strcpy(lock_path, filepath), lock_suffix);
	/* .lock Datei prüfen/erstellen */
	if ((fd_lock = open(lock, O_RDWR | O_CREAT | O_EXCL, 0640)) < 0){	
		perror("Lock-Datei vorhanden... checking");
		
		fd_lock = open(lock, O_RDWR, 0640);
		if ((read(fd_lock, pid_buffer, 10)) < 0){
			perror("error read");
			exit(-1);
		}
		pid = atoi(pid_buffer);
		printf("Locked by: %d\n", pid),
		result = waitpid(atoi(pid_buffer), &status, WNOHANG);
		printf("PID: %d, Status: %d\n", result, status);
		
		
		if(kill(pid, 0) == 0) {
			locked_message = "-ERR Mailbox locked\n";
			write(out, locked_message, strlen(locked_message));
			exit(0);
		} else {
			my_printf("Overwriting Lock-File, Process dead");
		}
	}
	sprintf(pid_buffer, "%d", getpid());
	write(fd_lock, pid_buffer, strlen(pid_buffer));
	
	return 0;
}

/* open user mailbox */
int open_mailbox(char *user) {
	DBRecord *record = malloc(sizeof(DBRecord));
	char *path_to_mb = malloc(DB_VALLEN);
	strcpy(record -> cat, cat_mailbox);
	strcpy(record -> key, user);
	db_search(path, 0, record);
	strcpy(path_to_mb, record -> value);
	process_lock(path_to_mb);
	fi = fi_new(path_to_mb, seperator);
	free(record);
	return 0;
}

/* login user */
int login_user(char *user, char *password) {
	DBRecord *record = malloc(sizeof(DBRecord));
	int result;
	strcpy(record -> cat, cat_password);
	strcpy(record -> key, user);
	db_search(path, 0, record);
	result = !strcmp(record -> value, password);
	free(record);
	return result;
}

/* pass */
int authenticate(DialogRec *d){
	int result = validate_noparam(d);
	if (result != 0){
		result = login_user(dialogs[0].param, dialogs[1].param);
		if(result) {
			open_mailbox(dialogs[0].param);
			write(out, logged_in, strlen(logged_in));
		}
	}
	return result;
}

/* user */
int get_user(DialogRec *d) {
	int result = validate_noparam(d);
	if (result != 0){
		write(out, ok, strlen(ok));
	}
	return result;
}

int display_dialogs(DialogRec dialogspec []){
	
	int counter = 0;
	DialogRec *dialog = &dialogspec[counter];
	
	while(strcmp(dialog -> command, "") != 0){
		printf("CMD: %s ", dialog -> command);
		printf("PARAM: %s ", dialog -> param);
		printf("VALID: %d\n", dialog -> is_valid);
		counter += 1;
		dialog = &dialogspec[counter];
	}
	
	return 0;
}

int process_pop3(int infd, int outfd) {

	ProlResult prolresult;
	int linemax = 1024;	
	int state = 0;
	char *line;
	LineBuffer *b;
	out = outfd;
	in = infd;
	write(outfd, pop3_ready, strlen(pop3_ready));
	
	while(1){
		line = calloc(1, linemax);
		b = buf_new(infd, "\r");
		buf_readline(b, line, linemax);
		prolresult = processLine(line, state, dialogs);
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
			write(outfd, err, sizeof(err));
		}
	}
	
	return 0;
}

int show_entries(FileIndex *fi){
	FileIndexEntry *fie = (fi -> entries);
	while(fie) {
		printf("Entry %d - Size: %d / Lines: %d / Seek: %d\n", fie ->nr, fie -> size, fie -> lines, fie -> seekpos);
		fie = fie -> next;
	}
	return 1;
}
