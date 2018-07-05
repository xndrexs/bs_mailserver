#include <stdio.h>
#include <stdlib.h>
#include "dialog.h"
#include <strings.h>
#include <string.h>

/* Gibt einen Buchstaben als Kleinbuchstaben zurück */
char charToLowerCase(char c) {
	
	char lower = c;
	if (c >= 'A' && c <= 'Z') {
		lower += 32;
	}
	return lower;
}

char *splitStringAtCommand(char *s1, char *s2){
	
	char *param = "\0";
	
	while(charToLowerCase(*s1) == charToLowerCase(*s2)) {
		s1++;
		s2++;
	}
	if (*s2){
		s2++;
		param = s2;
	}
	return param;
	
}

/* Prüft, ob s1 in s2 vorhanden ist oder */
int compare_string(char *s1, char *s2) {

	while(*s1) {
		if (charToLowerCase(*s1) != charToLowerCase(*s2)) {
			return 1;
		}
		s1++;
		s2++;
	}
	return 0;

}

/*
 * Findet ein DialogRec zum übergebenen State
 */
DialogRec *findCorrectdialogRecForState(int state, DialogRec dialogspec[]){
	int counter = 0;
	DialogRec *dialog = &dialogspec[counter];
	while(strcmp("", dialog -> command) != 0){
		if (dialog -> state == state) {
			return dialog;
		}
		counter += 1;
		dialog = &dialogspec[counter];
	}
	return NULL;
}

/*
 * processLine() zerlegt 'line' in einen Kommando- und (optional)
 * Parameter-Teil und kopiert Parameter in 'param'-Komponente
 * des zugehoerigen DialogRec im uebergebenen Array 'dialogspec'.
 * Arrayende ist wieder durch den Leerstring in 'command' markiert.
 * Ergebnis ist gefuellte ProlResult-struct. 'info' enthaelt eine
 * optionale Rueckmeldung fuer den Fehlerfall (failed != 0).
 * In 'state' wird der aktuelle Sytemzustand uebergeben.
 */
ProlResult processLine(char line[LINEMAX], int state, DialogRec dialogspec[]){
	
	ProlResult prolresult;
	DialogRec *dialog = findDialogRec(line, dialogspec);
	char *param = "\n";
	int valid;
	
	/* DialogRec gefunden */
	if (dialog != NULL) {
		/* State setzen, wenn Dialog gefunden wurde */
		if (dialog -> state == state) {
			prolresult.dialogrec = dialog;
			prolresult.failed = 0;
			strcpy(prolresult.info, "Command und State stimmen\n");
			
			/* Command und Param splitten */
			param = splitStringAtCommand(dialog -> command, line);
			strcpy(dialog -> param, param);	
			
			valid = 1;
			
			/* Validator ausführen */
			if (dialog -> validator != NULL){
				valid = dialog -> validator(dialog);
			}
			dialog -> is_valid = valid;
			if (valid == 0){
				prolresult.failed = 1;
			}
			if(valid == -17){
				prolresult.failed = -17;
			}
			
			
		} else {
			/*prolresult.dialogrec = dialog;*/
			prolresult.dialogrec = findCorrectdialogRecForState(state, dialogspec);
			prolresult.failed = 1;
			strcpy(prolresult.info, "Command stimmt, State nicht\n");
		}
		

		
	/* DialogRec nicht gefunden */			
	} else {
		prolresult.failed = 2;
		prolresult.dialogrec = NULL;
		strcpy(prolresult.info, "Command nicht gefunden\n");
	}
	
	return prolresult;
}

/*
 * findDialogRec() sucht DialogRec zu Kommando 'command' in 
 * Array 'dialogspec' und liefert Zeiger darauf oder NULL, 
 * falls nicht gefunden. Das Ende des Arrays 'dialogspec' 
 * enthaelt den Leerstring in der 'command'-Komponente.
 */
DialogRec *findDialogRec(char *command, DialogRec dialogspec[]) {
	
	int counter = 0;
	DialogRec *dialog = &dialogspec[counter];
	while(strcmp(dialog -> command, "") != 0){
		if (compare_string(dialog -> command, command) == 0){
			return dialog;
		}
		counter += 1;
		dialog = &dialogspec[counter];
	}
	return NULL;
}
