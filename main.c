#include <stdio.h>
#include <stdlib.h>
#include "server.h"

int main(int argv, char** argc) {

	if (argv != 2) {
		fprintf(stderr, "Not enough args\n");
		exit(1);
	}

	serv_t* server = Srv_NewServer(argc[1]);
	Srv_DoTheThing(server);

	return 0;

}
