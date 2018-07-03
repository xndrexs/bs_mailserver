# compiler
CC = gcc

# flags
CFLAGS = -g -ansi -pedantic -Wall -pthread

# file
pop3: mailserver.c pop3.c dialog.c fileindex.c linebuffer.c database.c smtp.c
	$(CC) $(CFLAGS) -o mail mailserver.c pop3.c dialog.c fileindex.c linebuffer.c database.c smtp.c
	
database: db.c database.c
	$(CC) $(CFLAGS) -o db db.c database.c
	
fi: fileindex.c linebuffer.c
	$(CC) $(CFLAGS) -o fi fileindex.c linebuffer.c
