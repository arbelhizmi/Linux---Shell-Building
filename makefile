all: ex1.c
	gcc ex1.c -o ex1
all-GDB: ex1.c
	gcc -g ex1.c -o ex1