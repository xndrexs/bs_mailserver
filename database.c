#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "database.h"

#include <sys/stat.h>
#include <sys/types.h>

/*char *filepath_tmp = "/home/mi/apoeh001/semester6/betriebssysteme/mailserver/database/database_tmp";*/
char *filepath_tmp = "/home/andreas/semester6/betriebssysteme/bs_mailserver/database/database_tmp";


int get_filesize(const char *path){
	struct stat buf;
	if (stat(path, &buf) < 0){
		perror("Fehler bei Dateigröße");
	}
	return buf.st_size;
}

/*
 * Gibt 1 zurück, wenn der Index zu groß ist oder kleiner 0, ansonsten 0
 */
int check_filesize(const char *path, int index){
	if ((index+1) * sizeof(DBRecord) > get_filesize(path) || index < 0){
		printf("Index out of Range\n");
		return 1;
	}
	return 0;
}

int db_list(const char *path, int outfd, int (*filter)(DBRecord *rec, const void *data), const void *data) {

	int fd_read = 0;
	char out[sizeof(DBRecord) + 7] = {0};
	
	/* Eingelesene Datei */
	int fd_open = open(path, O_RDONLY);
	if (fd_open < 0){
		perror("Fehler beim Lesen");
		return -1;
	}
	
	printf("\n===================== DATEBASE START =====================\n\n");
	while(1){
		DBRecord record;
		fd_read = read(fd_open, &record, sizeof(DBRecord));
		
		if(fd_read == 0) {
			break;
		}
		
		if (filter == NULL || (filter(&record, data) == 0)) {
			sprintf(out, "%-*s | %-*s | %s\n", DB_KEYLEN, record.key, DB_CATLEN, record.cat, record.value);
			write(outfd, out, DB_KEYLEN + DB_CATLEN + strlen(record.value) + 7);
		}
	}	
	printf("\n===================== DATEBASE END =====================\n\n");
	
	return fd_read;
}

int db_search(const char *filepath, int start, DBRecord *record) {
	
	DBRecord result;
	int index = 0;
	int fd_open, fd_read = 0;
	
	if (check_filesize(filepath, start) != 0){
		return -1;
	}
	
	/* Öffnen */
	fd_open = open(filepath, O_RDONLY);
	if (fd_open < 0){
		perror("Fehler beim Lesen");
		return -1;
	}
	
	/* Startposition festlegen */
	if (lseek(fd_open, start * sizeof(DBRecord), SEEK_SET) < 0) {
		perror("Fehler beim Positionieren");
		return -42;
	}
	
	while (1) {
		fd_read = read(fd_open, &result, sizeof(DBRecord));
		if (fd_read < 0){
			perror("Fehler");
			return -42;
		}
		if (fd_read == 0){
			break;
		}
		if((strcmp(result.key, record -> key) == 0 && strcmp(result.cat, record -> cat) == 0) ||
			(strcmp(record -> key, "") == 0 && strcmp(record -> cat, result.cat) == 0) ||
			(strcmp(record -> cat, "") == 0 && strcmp(record -> key, result.key) == 0)) {
				
			strcpy(record -> key, result.key);
			strcpy(record -> cat, result.cat);
			strcpy(record -> value, result.value);
			return index;
		}
		index++;
	}
	return -1;
}

/*
 * SEEK_SET: neue Position wird auf offset gesetzt 
 * SEEK_CUR: neue Position ist aktuelle Position + offset
 * SEEK_END: neue Position ist Dateiende + offset
 */
int db_get(const char *filepath, int index, DBRecord *result) {
	
	int fd_open = 0;
	if (check_filesize(filepath, index)){
		return 1;
	} 
	
	fd_open = open(filepath, O_RDONLY);
	if (fd_open < 0){
		perror("Fehler beim Lesen");
		return -1;
	}
	
	if (lseek(fd_open, index * sizeof(DBRecord), SEEK_SET) < 0) {
		perror("Fehler beim Positionieren");
		return -1;
	}
	
	if (read(fd_open, result, sizeof(DBRecord)) < 0){
		perror("Fehler beim Schreiben");
		return -1;
	}
	
	return 0;
}

int db_put(const char *filepath, int index, const DBRecord *record) {

	int fd_open = open(filepath, O_RDWR);
	
	if (index == -1 || check_filesize(filepath, index)){
		lseek(fd_open, 0, SEEK_END);
	} else {
		lseek(fd_open, index * sizeof(DBRecord), SEEK_SET);
	}
	
	if (write(fd_open, record, sizeof(DBRecord)) < 0){
		perror("error, db_put");
	}
	
	return 0;
}

int db_update(const char *filepath, const DBRecord *new) {

	DBRecord result;
	int index = 0;
	int fd_read = 0;
	
	int fd_open = open(filepath, O_RDWR);
	if (fd_open < 0){
		perror("Fehler beim Lesen");
		return -1;
	}
	
	while (1) {
		fd_read = read(fd_open, &result, sizeof(DBRecord));
		if (fd_read < 0){
			perror("Fehler");
			return -1;
		}
		if (fd_read == 0){
			break;
		}
			
		if(strcmp(result.key, new -> key) == 0 && strcmp(result.cat, new -> cat) == 0) {	
			printf("MATCH\n");
			write(fd_open, new, sizeof(DBRecord));
			return index;
		}
		index++;
	}
	return db_put(filepath, index, new);
}

int db_del(const char *filepath, int index) {
	
	char buffer[sizeof(DBRecord)];
	int fd_read = 0;
	int fd_copy = 0;
	int fd_open = 0;
	int skip_index = 0;
	if (fd_open < 0){
		perror("db_del");
	}
	if (check_filesize(filepath, index) != 0){
		return -1;
	}
	
	fd_open = open(filepath, O_RDWR);
	if (fd_open < 0){
		perror("db_del, open");
	}
	
	fd_copy = open(filepath_tmp, O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (fd_copy < 0){
		perror("db_del, copy");
	}
	
	while(1){
		if(skip_index == index){
			printf("Deleting Index: %d\n", skip_index);
			lseek(fd_open, sizeof(DBRecord), SEEK_CUR);
		}
		fd_read = read(fd_open, buffer, sizeof(DBRecord));
		if(fd_read == 0){
			break;
		}
		if (write(fd_copy, buffer, fd_read) < 0){
			perror("db_del, write");
		}
		
		skip_index++;
	}	
	
	rename(filepath_tmp, filepath);
	
	return 0;
}
