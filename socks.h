#ifndef SOCKS_H_
#define SOCKS_H_

#define MAX_FNAME 256
#define SOCKS_UNINIT_SD -1

/*
Create a new UDP socket and bind port to SERVER_PORT.
Inform user of address
*/
extern int UDP_socks_servinit(char *progname, 
                              struct sockaddr_in *self,
                              char *sname);
/*
Create a new UDP socket and "connect" to socket specified by sname and port
*/
extern int UDP_socks_clntinit(char *sname,
                              unsigned short port,
                              int rcvbuf);
/*
Create a new TCP socket and bind port to SERVER_PORT.
Inform user of address and listen for connections
*/
extern int TCP_socks_servinit(char *progname, 
                              struct sockaddr_in *self,
                              char *sname, 
                              int reuse);

/* 
Create a new TCP socket and connect to socket specified by server.
If server->sin_addr.s_addr is NULL, obtain address from sname.
Server.sin_port must be specified when calling. If connection fails,
return SOCKS_UNINIT_SD, else inform user of address and listen for connections
*/
extern int TCP_socks_clntinit(struct sockaddr_in *server,
                              char *sname,
                              int reuse,
                              struct sockaddr_in *self);
/*
Accept connection on socket sd
Print info of client that connected
Return new socket for connectoin
*/
extern int TCP_socks_accept(int sd);

/*
Close socket td
*/
extern void socks_close(int td);


#endif /* SOCKS_H_ */
