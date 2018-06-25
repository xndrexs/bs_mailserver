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

FileIndex *fi_new(const char *filepath, const char *separator) {
	
	FileIndex *fi = malloc(sizeof(FileIndex));		/* Neuer FileIndex */
	FileIndexEntry *fie, *fie_cur;					/* Variable für Entries */
	LineBuffer *buf;								/* Variable für Buffer */
	int linemax = 1024;								/* Lange der Zeile */
	char *line = malloc(linemax);					/* Speicher für Line belegen */
	int fd_open = 0;								/* Deskriptor zum Öffnen der Datei */
	int fd_lock = 0;								/* Deskriptor für Lock-File */
	int nr = 1;										/* Variable zum Zählen der Abschnitte */
	char lock_path[100];	
	char *lock_suffix = ".lock";
	char pid_buffer[10];
	
	fi -> filepath = filepath;						/* Filepath merken */
	fi -> entries = NULL;
	fi -> totalSize = 0;

	

	/* .lock Datei prüfen/erstellen */
	if ((fd_lock = open(strcat(strcpy(lock_path, filepath), lock_suffix), O_RDWR | O_CREAT | O_EXCL)) < 0){
		perror("error open");
	} else {
		sprintf(pid_buffer, "%d", getpid());
		write(fd_lock, pid_buffer, strlen(pid_buffer));
		exit(-1);
	}

	if ((fd_open = open(filepath, O_RDWR)) < 0) {
		perror("Error Open");
	}
	

	
	buf = buf_new(fd_open, "\n");
	
	while(buf_readline(buf, line, linemax) != -1){
		/* Neuer Abschnitt */
		if(strncmp(line, separator, strlen(separator)) == 0) {
			
			fie_cur = malloc(sizeof(FileIndexEntry));
			fie_cur -> nr = nr;
			fie_cur -> next = NULL;
			fie_cur -> seekpos = buf_where(buf) - strlen(line) - 1;
			fie_cur -> del_flag = 0;
			/* 1. Eintrag */
			if ((fi -> entries) == NULL) {
				fi -> entries = fie_cur;
				fie = fie_cur;
			/* Weitere Einträge */
			} else {
				fie -> next = fie_cur;
				fie = fie_cur;				
			}
			fi -> nEntries = nr;
			nr++;
		}
		(fie -> lines)++;
		(fie -> size) += strlen(line) +1;
		(fi -> totalSize) += strlen(line) +1;
	}
	return fi;
}

void fi_dispose(FileIndex *fi) {
	FileIndexEntry *fie, *fie_cur;
	fie = (fi -> entries);
	while(fie) {
		fie_cur = fie;
		free(fie_cur);
		fie = fie -> next;
	}
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
	
	
	
	return 0;
}
