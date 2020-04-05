#include <stdio.h>
#include <stdlib.h>
#include "server.h"

int main(int argv, char** argc) {

	if (argv != 2) {
		fprintf(stderr, "Not enough args\n");
		exit(1);
	}

	fprintf(stderr, "MinChat Server!\n");

	serv_t* server = Srv_NewServer(argc[1]);
	printf("\E[34m[*]\E[0m New server created\n");

	Srv_DoTheThing(server);

	return 0;

}
