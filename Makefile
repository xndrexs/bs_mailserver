# compiler
CC = gcc

# flags
CFLAGS = -g -ansi -pedantic -Wall

# file
pop3: mailserver.c pop3.c dialog.c fileindex.c linebuffer.c database.c
	$(CC) $(CFLAGS) -o mail mailserver.c pop3.c dialog.c fileindex.c linebuffer.c database.c
	
database: db.c database.c
	$(CC) $(CFLAGS) -o db db.c database.c
