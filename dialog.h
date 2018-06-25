#ifndef DIALOG_H
#define DIALOG_H

enum { CMDMAX=17, PARAMMAX=80, LINEMAX=100 };

typedef struct drec {
    char command[CMDMAX];   /* Kommando-String, case-insensitiv */
    char param[PARAMMAX];   /* Parameter-String */
    int  state;             /* notwendiger Systemzustand-Level */
    int  nextstate;         /* Folgezustand nach diesem Kommando */
    int (*validator)(struct drec *); /* Validatorfktn oder NULL */
    char is_valid;          /* ist dieses Record ist gueltig? */
} DialogRec;

/*
 * findDialogRec() sucht DialogRec zu Kommando 'command' in 
 * Array 'dialogspec' und liefert Zeiger darauf oder NULL, 
 * falls nicht gefunden. Das Ende des Arrays 'dialogspec' 
 * enthaelt den Leerstring in der 'command'-Komponente.
 */
DialogRec *findDialogRec(char *command, DialogRec dialogspec[]);

/*
 * processLine() zerlegt 'line' in einen Kommando- und (optional)
 * Parameter-Teil und kopiert Parameter in 'param'-Komponente
 * des zugehoerigen DialogRec im uebergebenen Array 'dialogspec'.
 * Arrayende ist wieder durch den Leerstring in 'command' markiert.
 * Ergebnis ist gefuellte ProlResult-struct. 'info' enthaelt eine
 * optionale Rueckmeldung fuer den Fehlerfall (failed != 0).
 * In 'state' wird der aktuelle Sytemzustand uebergeben.
 */
enum {GRMSGMAX = 120};
typedef struct pr {
    char failed;            /* processLine() erfolgreich? */
    DialogRec *dialogrec;   /* Bezugs-DialogRec */
    char info[GRMSGMAX];    /* Info-String an Aufrufer */
} ProlResult;

ProlResult processLine(char line[LINEMAX], int state, DialogRec dialogspec[]);

#endif

