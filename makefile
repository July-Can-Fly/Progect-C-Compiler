assembler: assembler.o code.o
	gcc -g -ansi -Wall -pedantic assembler.o code.o -o assembler
assembler.o: assembler.c code.h
	gcc -c -g -ansi -Wall -pedantic assembler.c -o assembler.o
code.o: code.c code.h
	gcc -c -g -ansi -Wall -pedantic code.c -o code.o

