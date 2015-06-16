all:
	gcc -c -g lfsreader.c -Wall
	gcc -g -o lfsreader lfsreader.o -Wall
clean:
	rm -f lfsreader.o
