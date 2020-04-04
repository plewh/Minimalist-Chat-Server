all: main.c server.c
	gcc -std=gnu99 main.c server.c -o cht
