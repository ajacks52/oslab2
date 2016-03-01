CC=gcc
CFLAGS=-g -std=gnu99
RM=/bin/rm -f

INCLUDE = -Iaux_files/

simsh1: chop_line.h chop_line.c simsh1.c
	$(CC) $(CFLAGS) -o simsh1 simsh1.c chop_line.c

simsh2: chop_line.h chop_line.c simsh2.c
	$(CC) $(CFLAGS) -o simsh2 simsh2.c chop_line.c

simsh3: chop_line.h chop_line.c simsh3.c
	$(CC) $(CFLAGS) -o simsh3 simsh3.c chop_line.c

clean:
	$(RM) simsh1 *~
	$(RM) simsh2 *~
	$(RM) simsh3 *~
