#include "fileindex.h"
#include "linebuffer.h"
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

char *mailbox_tmp = "/home/mi/apoeh001/semester6/betriebssysteme/mailserver/mailbox/mailbox_tmp";

FileIndex *fi_new(const char *filepath, const char *separator) {
	
	FileIndex *fi = malloc(sizeof(FileIndex));		/* Neuer FileIndex */
	FileIndexEntry *fie, *fie_cur;					/* Variable für Entries */
	LineBuffer *buf;								/* Variable für Buffer */
	int linemax = 1024;								/* Lange der Zeile */
	char *line = malloc(linemax);					/* Speicher für Line belegen */
	int fd_open = 0;								/* Deskriptor zum Öffnen der Datei */
	int size = 0, seek = 0, nr = 1;								
	fi -> filepath = filepath;						
	fi -> entries = NULL;
	fi -> totalSize = 0;

	/* Mailbox öffnen */
	if ((fd_open = open(filepath, O_RDWR)) < 0) {
		perror("Error Open");
	}
	
	buf = buf_new(fd_open, "\n");
	
	while((seek = buf_readline(buf, line, linemax)) != -1){
		/* Neuer Abschnitt */
		if(strncmp(line, separator, strlen(separator)) == 0) {
			fie_cur = calloc(1, sizeof(FileIndexEntry));
			fie_cur -> nr = nr;
			fie_cur -> next = NULL;
			fie_cur -> del_flag = 0;
			fie_cur -> seekpos = seek;
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
		}
		(fie -> lines)++;
		size = strlen(line)+1;
		(fie -> size) += size;
		(fi -> totalSize) += size;
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
	int fd_open = 0, fd_write = 0, fd_copy = 0, fd_read = 0;
	FileIndexEntry *fie = (fi -> entries);
	char *buffer;
	
	if((fd_open = open(fi -> filepath, O_RDWR)) < 0) {
		perror("error open mb");
	}
	
	if((fd_copy = open(mailbox_tmp, O_RDWR | O_CREAT | O_TRUNC, 0640)) < 0){
		perror ("error open tmp");
	}
	
	while(fie) {

		if ((fie -> del_flag) == 0) {
			lseek(fd_open, fie -> seekpos, SEEK_SET);
			buffer = calloc(1, fie -> size);
			if ((fd_read = read(fd_open, buffer, fie -> size)) < 0){
				perror("error read");
			}
			if((fd_write = write(fd_copy, buffer, fie -> size)) < 0){
				perror("error write");
			}
			write(fd_copy, "\n", 1);	
			free(buffer);
		}
		
		fie = fie -> next;
	}
	rename(mailbox_tmp, fi -> filepath);
	return 0;
}
