#pragma once
#include <netdb.h>

#define MAX_PORT_LEN   10
#define MAX_RCONNS     10
#define MAX_RECV_BYTES 1400
#define MAX_BUF        1400

typedef struct {

	char prevHandle[MAX_BUF];
	char handle[MAX_BUF];

} rhost_t;

typedef struct {

	rhost_t*          rhost[MAX_RCONNS];// remote host info
	char*             lhost_port;		// port that server listens on

	struct addrinfo   lhost_hints;		// the requirements we need for our socket
	struct addrinfo*  lhost_addr;		// the actual socket info
	int               lhost_listen_fd;	// the fd bound to the socket
	fd_set            mSet;				// master set of fd's
	int               mSetMax;			// largest fd in use so far
	struct addrinfo   rhost_addr;		// the remote host socket info

} serv_t;

serv_t*  Srv_NewServer  (char* port);
void     Srv_DoTheThing (serv_t* server);
void     Srv_NewRhost   (serv_t* server, int fd);
void     Srv_SendMess   (serv_t* server, char* buf, int recv_fd);
void     Srv_FormMess   (serv_t* server, char* buf, char* nBuf, int recv_fd);
void     Srv_SendMotd   (serv_t* server, int fd);
void     Srv_SendWelc   (serv_t* server, int fd);
void     Srv_SendHandleUpdate(serv_t* server, int fd);
