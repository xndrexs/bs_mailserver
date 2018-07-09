#include "fileindex.h"
#include "linebuffer.h"
#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <string.h>

/*
typedef struct fie
{
  struct fie *next;
  int seekpos;          lseek()-Position des ersten Inhalts-Bytes des Abschnitts
  int size;             Groesse des Abschnitt-Inhalts in Bytes, incl. Zeilentrenner
  int lines;            Anzahl Inhalts-Zeilen im Abschnitt
  int nr;               laufende Nummer des Abschnitts, beginnend bei 1
  int del_flag;         Abschnitt zum Loeschen vormerken
} FileIndexEntry;

typedef struct fi
{
  const char *filepath;     Dateipfad der zugehoerigen Datei
  FileIndexEntry *entries;  Liste der Abschnittsbeschreibungen
  int nEntries;             Gesamtanzahl Abschnitte
  int totalSize;            Summe der size-Werte aller Abschnitte
} FileIndex;
*/

FileIndex *fi_new(const char *filepath, const char *separator) {
	
	FileIndex *fi = malloc(sizeof(FileIndex));		/* Neuer FileIndex */
	FileIndexEntry *fie, *fie_cur;					/* Variable für Entries */
	LineBuffer *buf;								/* Variable für Buffer */
	int linemax = 1024, size = 0, seek = 0, nr = 1, head = 0;
	char *line = malloc(linemax);					/* Speicher für Line belegen */
	int fd_open = 0;
								
	fi -> filepath = filepath;						
	fi -> entries = NULL;
	fi -> totalSize = 0;

	/* Mailbox öffnen */
	if ((fd_open = open(filepath, O_RDONLY)) < 0) {	perror("Error Open"); }
	
	buf = buf_new(fd_open, "\n");
	
	while((seek = buf_readline(buf, line, linemax)) != -1){
		/* Neuer Abschnitt */
		if(strncmp(line, separator, strlen(separator)) == 0) {
			fie_cur = calloc(1, sizeof(FileIndexEntry));
			fie_cur -> nr = nr;
			fie_cur -> next = NULL;
			fie_cur -> del_flag = 0;
			head = 1;
			/* 1. Eintrag */
			if ((fi -> entries) == NULL) {
				fi -> entries = fie_cur;
				fie = fie_cur;
			/* Weitere Einträge */
			} else {
				fie -> size -= 1;
				fie -> next = fie_cur;
				fie = fie_cur;				
			}
			fi -> nEntries = nr;
			nr++;
		} else {
			if (head == 1) {
				fie -> seekpos = seek;
				head = 0;
			} 
			size = strlen(line)+1;
			(fie -> lines)++;
			(fie -> size) += size;
			(fi -> totalSize) += size;
		}
	}
	free(line);
	free(buf);
	return fi;
}

void fi_dispose(FileIndex *fi) {
	FileIndexEntry *fie, *fie_cur;
	fie = (fi -> entries);
	while(fie) {
		fie_cur = fie;
		fie = fie -> next;
		free(fie_cur);
	}
	free((char*)(fi -> filepath));
	free(fi);
}

FileIndexEntry *fi_find(FileIndex *fi, int n) {
	FileIndexEntry *fie = (fi -> entries);
	while(fie) {
		if (fie -> nr == n) {
			return fie;
		}
		fie = fie -> next;
	}
	return NULL;
}

int fi_compactify(FileIndex *fi) {
	int fd_open = 0, fd_write = 0, fd_copy = 0;
	int changed = 0, seek = 0, linemax = 1024;
	char *line = malloc(linemax);
	FileIndexEntry *fie = (fi -> entries);
	LineBuffer *buf;
	
	/* Prüfen, ob überhaupt eine Mail gelöscht werden soll */
	while(fie) {
		if ((fie -> del_flag) != 0) {
			changed = 1;
			break;
		}
		fie = fie -> next;
	}
	
	if (changed == 0) {
		my_printf("Nothing to delete.");
		return 0;
	}
	
	/* Wenn etwas gelöscht werden muss */
	fie = (fi -> entries);
	
	if((fd_open = open(fi -> filepath, O_RDWR)) < 0) { perror("error open mb");	}
	if((fd_copy = open(mailbox_tmp, O_RDWR | O_CREAT | O_TRUNC, 0640)) < 0){ perror ("error open tmp");	}
	
	buf = buf_new(fd_open, "\n");
	
	/* Zeilenweise kopieren */
	while((seek = buf_readline(buf, line, linemax)) != -1) {
		
		if(fie -> next) {
			/* Bedingung erfüllt, wenn die aktuelle Line die From-Zeile ist */
			if(seek + strlen(line) + 1 >= ((fie -> next) -> seekpos)) {
				fie = fie -> next;
				printf("line %s\n", line);
				printf("seek: %d seekpos: %d\n", seek, fie -> seekpos);	
			}		
		}
		
		if((fie -> del_flag) == 0){
			if((fd_write = write(fd_copy, line, strlen(line))) < 0) { perror("error write"); }
			if((fd_write = write(fd_copy, "\n", 1)) < 0) { perror("error write"); }
		}
	}
	
	free(line);
	rename(mailbox_tmp, fi -> filepath);
	remove(mailbox_tmp);
	return 0;
}
