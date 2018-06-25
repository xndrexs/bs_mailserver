#include <stdio.h>
#include <stdlib.h>

#include "database.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <string.h>


const char *path = "/home/mi/apoeh001/semester6/betriebssysteme/mailserver/database/database";
/* const char *path = "/home/andreas/semester6/betriebssysteme/mailserver/database/database"; */
const char *tmp_path = "/home/mi/apoeh001/semester6/betriebssysteme/ueb3/database/database_tmp";

int show_dbrecord(DBRecord *rec){
	char out[sizeof(DBRecord) + 7] = {0};
	sprintf(out, "%-*s | %-*s | %s\n", DB_KEYLEN, rec -> key, DB_CATLEN, rec -> cat, rec -> value);
	write(1, out, DB_KEYLEN + DB_CATLEN + strlen(rec -> value) + 7);
	/*printf("Key: %s\nCat: %s\nValue: %s\n", rec -> key, rec -> cat, rec -> value);*/
	return 0;
}

int create_database(const char *path){
	
	int fd_open = 0;
	fd_open = open(path, O_RDWR | O_CREAT | O_APPEND, 0640);
	
	/*
	DBRecord recs [] = {
		{"meier", "glupschi", "BlubberSuite"},
		{"meier", "lupschi", "Gruppenaquarium"},
		{"huber", "fluppi", "Loftiquarium"},
		{"schmidt", "ronny", "Gruppenaquarium"},
		{"mueller", "gurki", "Fischglas"},
		{"bauer", "pepino", "Waschbecken"},
		{"florian", "supi", "Teekanne"}
	};
	
	if (write(fd_open, recs, sizeof(recs)) < 0){
		perror("write_test_data");
	}
	*/	
	return 0;
}

int run_tests(const char *path_in){
	
	DBRecord get_rec;
	DBRecord put_rec = {"5", "Cat5", "Value5"};
	DBRecord search_rec = {"4", "", "Some Value"};
	DBRecord update_rec = {"23", "Cat2", "New Value2"};
	
	int fd_out = 1;
	
	printf("Aufgabe 1\n");
	
	db_list(path_in, fd_out, NULL, NULL);
	
	printf("\nGet: \n");
	db_get(path_in, 3, &get_rec);
	show_dbrecord(&get_rec);
	
	printf("\nPut: \n");
	db_put(path_in, 10, &put_rec);
	db_list(path_in, fd_out, NULL, NULL);
	
	printf("\nSearch: \n");
	db_search(path_in, 0, &search_rec);
	show_dbrecord(&search_rec);
	
	printf("\nUpdate: \n");
	db_update(path_in, &update_rec);
	db_list(path_in, fd_out, NULL, NULL);
	
	printf("\nDelete: \n");
	db_del(path_in, 2);
	db_list(tmp_path, fd_out, NULL, NULL);
	
	return 0;
}

int filter(DBRecord *rec, const void *data) {
	/*printf("Key: %s / Data: %s", rec -> key, data);*/
	return (strcmp(rec -> key, data) != 0);
}

int handle_command(const char *path, const char *cmd, char *data[]) {
	
	DBRecord *result = malloc(sizeof(DBRecord));
	int index = 0;
	int counter = 0;
	
	/* list */
	if (strcmp(cmd, "list") == 0){
		if (*data){
			db_list(path, 1, filter, *data);
		} else {		
			db_list(path, 1, NULL, NULL);
		}
	}
	
	/* search */
	if (strcmp(cmd, "search") == 0){
		strcpy(result -> key, *data);
		index = db_search(path, 0, result);
		show_dbrecord(result);
	}
	
	/* add */
	if (strcmp(cmd, "add") == 0){

		if (*(data+2)){
			strcpy(result -> key, *data);
			strcpy(result -> cat, *(data+1));
			strcpy(result -> value, *(data+2));
			index = db_search(path, 0, result);
			show_dbrecord(result);
			strcpy(result -> key, *data);
			strcpy(result -> cat, *(data+1));
			strcpy(result -> value, *(data+2));
			db_put(path, index, result); 
		} else {
			strcpy(result -> key, *data);
			strcpy(result -> cat, *(data+1));
			strcpy(result -> value, "Gruppenaquarium");
			db_put(path, -1, result);
		}
	}
	
	/* update */
	if (strcmp(cmd, "update") == 0){
		while (1) {
			strcpy(result -> key, *data);
			strcpy(result -> cat, "");
			index = db_search(path, counter, result);
			counter++;
			if (index < 0){
				break;
			}
			strcpy(result -> value, *(data+1));
			db_update(path, result);
		}
	}
	
	/* delete */
	if (strcmp(cmd, "delete") == 0){
		if (*(data+1)) {
			strcpy(result -> key, *data);
			strcpy(result -> cat, *(data+1));
		} else {
			strcpy(result -> key, *data);
		}
		while((index = db_search(path, 0, result)) >= 0) {
			db_del(path, index);
		}
	}
	
	/* db_list(path, 1, NULL, NULL); */
	return 0;
}


int main(int argc, char *argv[]){
	const char *cmd = *(argv+1);
	char **arg = argv;
	
	if (argc < 2) {
		printf("No args :( ! \n"); 
		exit(1);
	}
	
	printf("argc: %u\n", argc);
	while(*arg){
		printf("argv: %s\n", *arg); 
		arg += 1;
	}
	
	create_database(path);
	/*run_tests(path);*/
	handle_command(path, cmd, argv+2);
	return 0;
}