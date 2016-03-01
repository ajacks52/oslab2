CC=gcc
CFLAGS=-g -std=gnu99
RM=/bin/rm -f

INCLUDE = -Iaux_files/

simsh1: simsh1.c
	$(CC) -o simsh1 $(CFLAGS)  simsh1.c

simsh2: simsh2.c
	$(CC) $(CFLAGS) -o simsh2 simsh2.c

simsh3: simsh3.c
	$(CC) $(CFLAGS) -o simsh3 simsh3.c

clean:
	$(RM) simsh1 *~
	$(RM) simsh2 *~
	$(RM) simsh3 *~
