#ifndef DATABASE_H
#define DATABASE_H


enum { DB_KEYLEN=20, DB_CATLEN=20, DB_VALLEN=255 };

typedef struct dbrec {
    char key[DB_KEYLEN];     /* Schluessel */
    char cat[DB_CATLEN];     /* Kategorie */
    char value[DB_VALLEN];   /* Wert */
} DBRecord;

int db_list(const char *path, int outfd, 
  int (*filter)(DBRecord *rec, const void *data), const void *data);

int db_search(const char *filepath, int start, DBRecord *record);

int db_get(const char *filepath, int index, DBRecord *result);

int db_put(const char *filepath, int index, const DBRecord *record);

int db_update(const char *filepath, const DBRecord *new);

int db_del(const char *filepath, int index);

#endif
