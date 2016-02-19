CC=gcc
CFLAGS=-g -std=c99
RM=/bin/rm -f

INCLUDE = -Iaux_files/


huffencode: huffman.h huffman.c huffencode.c tree_pqueue.h tree_pqueue.c
	gcc -o huffencode huffman.c huffencode.c tree_pqueue.c

simsh1: simsh1.c
	$(CC) -o simsh1 $(CFLAGS)  simsh1.c

simsh2: simsh2.c
	$(CC) $(CFLAGS) $(INCLUDE) -o simsh2 simsh2.c

simsh3: simsh3.c
	$(CC) $(CFLAGS) $(INCLUDE) -o simsh3 simsh3.c

clean:
	$(RM) simsh1 *~
	$(RM) simsh2 *~
	$(RM) simsh3 *~
