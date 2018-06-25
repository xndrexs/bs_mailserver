#ifndef FILEINDEX_H
#define FILEINDEX_H

#define MAXDATEIPFAD 255

typedef struct fie
{
  struct fie *next;
  int seekpos;          /* lseek()-Position des ersten Inhalts-Bytes des Abschnitts */
  int size;             /* Groesse des Abschnitt-Inhalts in Bytes, incl. Zeilentrenner */
  int lines;            /* Anzahl Inhalts-Zeilen im Abschnitt */
  int nr;               /* laufende Nummer des Abschnitts, beginnend bei 1 */
  int del_flag;         /* Abschnitt zum Loeschen vormerken */
} FileIndexEntry;

typedef struct fi
{
  const char *filepath;     /* Dateipfad der zugehoerigen Datei */
  FileIndexEntry *entries;  /* Liste der Abschnittsbeschreibungen */
  int nEntries;             /* Gesamtanzahl Abschnitte */
  int totalSize;            /* Summe der size-Werte aller Abschnitte */
} FileIndex;


FileIndex *fi_new(const char *filepath, const char *separator);

void fi_dispose(FileIndex *fi);

FileIndexEntry *fi_find(FileIndex *fi, int n);

int fi_compactify(FileIndex *fi);

#endif
