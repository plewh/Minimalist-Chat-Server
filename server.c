#include "server.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_BACKLOG 30

serv_t* Srv_NewServer(char* port) {

	serv_t* s = malloc(sizeof(serv_t));

	s->lhost_port = port;
	for (int j = 0; j < MAX_RCONNS; ++j)
		s->rhost[j] = NULL;

	return s;

}

void Srv_DoTheThing(serv_t* server) {

	fprintf(stderr, "%s\n", server->lhost_port);

	// get list of available sockets (we'll always use the first one)
	memset(&server->lhost_hints, 0, sizeof(server->lhost_hints));
	server->lhost_hints.ai_family   = AF_UNSPEC;
	server->lhost_hints.ai_socktype = SOCK_STREAM;
	server->lhost_hints.ai_flags    = AI_PASSIVE;
	getaddrinfo(NULL, server->lhost_port, &server->lhost_hints, &server->lhost_addr);

	// open a socket, bind it to a port, use the fd to read/write network data
	server->lhost_listen_fd = socket(
		server->lhost_addr->ai_family, 
		server->lhost_addr->ai_socktype, 
		server->lhost_addr->ai_protocol
	);

	bind(
		server->lhost_listen_fd, 
		server->lhost_addr->ai_addr, 
		server->lhost_addr->ai_addrlen
	);

	// set port to LISTEN state (we're going to be a server)
	listen(server->lhost_listen_fd, MAX_BACKLOG);

	// add listener to master list of fds
	FD_SET(server->lhost_listen_fd, &server->mSet);
	server->mSetMax = server->lhost_listen_fd;

	while (1) {

		// make copy of master list
		fd_set tSet = server->mSet;

		// vars for handling recv'd data
		int nBytes = 0;
		char buf[MAX_RECV_BYTES];
		memset(buf, 0, MAX_RECV_BYTES);
		buf[0] = '\0';

		// select() walks set of fds, checks if fd has 
		// data available. If so, will set ISSET flag on that fd
		select(server->mSetMax + 1, &tSet, NULL, NULL, NULL);

		// now, WE need to walk set of fds looking for ISSET
		// for each fd in set...
		for (int i = 0; i <= server->mSetMax; i++) {

			// is data available on THIS fd?
			if (FD_ISSET(i, &tSet)) {

				// fd used for listening on port is special case:
				// if ISSET, someone is trying to connect
				if (i == server->lhost_listen_fd) {

					printf("\E[34m[*]\E[0m Caught new connection... ");

					// create a new socket & fd to communicate with rhost
					struct sockaddr_storage rhost_addr;
					socklen_t rhost_addr_len = sizeof(rhost_addr);

					int newfd = accept(
						server->lhost_listen_fd, 
						(struct sockaddr*)&rhost_addr, 
						&rhost_addr_len
					);

					// add new fd to master fd list
					FD_SET(newfd, &server->mSet);

					// keep track of list end
					if (newfd > server->mSetMax)
						server->mSetMax = newfd;

					printf(" assigned to fd %d\n", newfd);

					Srv_NewRhost(server, newfd);
					Srv_SendWelc(server, newfd);
					Srv_SendMotd(server, newfd);

				// this fd is not the listening port, so ISSET means rhost is sending data
				} else {

					//read data from fd
					if ((nBytes = recv(i, buf, sizeof(buf), 0)) <= 0) {

						// data recieved was the hang up signal...
						if (nBytes == 0)
							printf("\E[34m[*]\E[0m fd %d wants to hang up... ", i);
						// ...or data was an error signal
						else
							perror("recv");

						// either way, we need to close this connection
						close(i);
						FD_CLR(i, &server->mSet);
						printf("fd closed, Goodbye!\n");

					// recieved data isn't an error or hangup signal
					} else {

						printf("\E[32m[+]\E[0m Recieved %d bytes from fd %d\n", nBytes, i);

						// parse recv bytes and determine if this is a command or a message
						char* r = strstr(buf, "/name");
						if (r && ((r - buf) == 0 && *(r + 5) == ' ')) {

							// the cmd is valid: substring exists, it's at the start and has a space
							printf("\E[32m[+]\E[0m Caught /name cmd!\n");
							// remove newline from end of string
							r[strlen(r) - 1] = '\0';

							// get new handle
							char uHandle[MAX_BUF];
							memset(uHandle, 0, sizeof(uHandle));
							strcpy(uHandle, r + 6);

							// update handle associated with this fd
							strcpy(server->rhost[i]->prevHandle, server->rhost[i]->handle);
							strcpy(server->rhost[i]->handle, uHandle);
							
							// let everyone connected know of handle change
							Srv_SendHandleUpdate(server, i);

						} else {

							// construct message buffer
							char nBuf[MAX_BUF];
							memset(nBuf, 0, sizeof(nBuf));
							Srv_FormMess(server, buf, nBuf, i);

							// send message to all connected hosts
							Srv_SendMess(server, nBuf, i);

						}

					}

				}

			}

		}

	}

}

void Srv_NewRhost(serv_t* server, int fd) {

	rhost_t* r = malloc(sizeof(rhost_t));

	strcpy(r->prevHandle, "ANON");
	strcpy(r->handle, "ANON");

	server->rhost[fd] = r;

}

void Srv_SendMess(serv_t* server, char* buf, int recv_fd) {

	for(int j = 0; j <= server->mSetMax; ++j)
		if (FD_ISSET(j, &server->mSet))
			if (j != server->lhost_listen_fd && j != recv_fd)
				send(j, buf, strlen(buf), 0);

}

void Srv_FormMess(serv_t* server, char* buf, char* nBuf, int recv_fd) {

	// for each fd in list, send() the read data
	// (after checking we aren't sending to listening port
	// or sending back to the rhost that sent the data)
	strcpy(nBuf, "\E[32m");
	strcat(nBuf, server->rhost[recv_fd]->handle);
	strcat(nBuf, "\E[0m");
	strcat(nBuf, " | ");
	strcat(nBuf, buf); // todo: use strncat

}

void Srv_SendMotd(serv_t* server, int fd) {

	char motd[MAX_BUF];
	memset(motd, 0, sizeof(motd));
	strcpy(motd, "\E[33mSERV\E[0m | ");
	strcat(motd, "MOTD: That is not dead which can eternal lie, And with strange aeons even death may die.");
	strcat(motd, "\n");
	send(fd, motd, strlen(motd), 0);

}

void Srv_SendWelc(serv_t* server, int fd) {

	char welc[MAX_BUF];
	memset(welc, 0, sizeof(welc));
	strcpy(welc, "\E[33mSERV\E[0m | ");
	strcat(welc, "Welcome to MinChat Server");
	strcat(welc, "\n");
	send(fd, welc, strlen(welc), 0);

}

void Srv_SendHandleUpdate(serv_t* server, int fd) {

	char buf[MAX_BUF];
	memset(buf, 0, sizeof(buf));

	sprintf(
		buf, 
		"\E[33mSERV\E[0m | %s changed name to %s\n",
		server->rhost[fd]->prevHandle,
		server->rhost[fd]->handle
	);

	for(int j = 0; j <= server->mSetMax; ++j)
		if (FD_ISSET(j, &server->mSet))
			if (j != server->lhost_listen_fd && j != fd)
				send(j, buf, strlen(buf), 0);

}
