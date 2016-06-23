#include "socks_util.h"

#include "Phone.h"

#include <cstdio>
#include <cstring>

#include <signal.h>
#include <netdb.h>         // gethostbyname()
#include <netinet/in.h>    // struct in_addr
#include <arpa/inet.h>     // htons()
#include <sys/types.h>     // u_short
#include <sys/socket.h>    // socket API
#include <sys/ioctl.h>     // ioctl(), FIONBIO
#include <unistd.h>

/////////////////////////
//         UDP         //
/////////////////////////

int UDP_socks_servinit(char *progname, struct sockaddr_in *self, char *sname)
{
  int sd = -1;
  int err, len;
  struct hostent *sp;
  unsigned int localaddr;

  /* create a UDP socket */
  sd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  
  memset((char *) self, 0, sizeof(struct sockaddr_in));
  self->sin_family = AF_INET;
  self->sin_addr.s_addr = INADDR_ANY;
  self->sin_port = htons(SERVER_PORT);

  /* bind address to socket */
  err = bind(sd, (struct sockaddr *) self,
             sizeof(struct sockaddr_in));
  int val = 1;
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
  setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val));
  /*
   * Obtain the ephemeral port assigned by the OS kernel to this
   * socket and store it in the local variable "self".
   */
  len = sizeof(struct sockaddr_in);
  err = getsockname(sd, (struct sockaddr *) self, (socklen_t *) &len);

  /* Find out the FQDN of the current host and store it in the local
     variable "sname".  gethostname() is usually sufficient. */
  err = gethostname(sname, 256);

  /* Check if the hostname is a valid FQDN or a local alias.
     If local alias, set sname to "localhost" and addr to 127.0.0.1 */
  sp = gethostbyname(sname);
  if (sp) {
    memcpy(&self->sin_addr, sp->h_addr, (size_t)sp->h_length);
  } else {
    localaddr = (unsigned int) inet_addr("127.0.0.1");
    memcpy(&self->sin_addr, (char *) &localaddr, sizeof(unsigned int));
    strcpy(sname, "localhost");
  }
  
  /* inform user which port this peer is listening on */
  fprintf(stderr, "%s address is %s:%d\n",
          progname, sname, ntohs(self->sin_port));

  return sd;
}

int UDP_socks_clntinit(char *sname, unsigned short port, int rcvbuf) {
  int sd, err;
  struct sockaddr_in server;
  struct hostent *sp;
  socklen_t optlen;

  /* create a new UDP socket. */
  sd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);   // sd global

  /* obtain the server's IPv4 address from sname and initialize the
     socket address with server's address and port number . */
  memset((char *) &server, 0, sizeof(struct sockaddr_in));
  server.sin_family = AF_INET;
  server.sin_port = port;
  sp = gethostbyname(sname);
  memcpy(&server.sin_addr, sp->h_addr, (size_t)sp->h_length);

  /* 
   * set socket receive buffer size to be at least rcvbuf bytes.
  */
  err = setsockopt(sd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(int));
  optlen = sizeof(int);
  err = getsockopt(sd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &optlen);

  char fname[256];
  err = gethostname(fname, 256);
  
  /* inform user which port this peer is listening on */
  fprintf(stderr, "audio address is %s:%d\n",
          fname, ntohs(server.sin_port));

  fprintf(stderr, "socks_clntinit: socket receive buffer set to %d bytes\n", rcvbuf);
  
  /* since this is a UDP socket, connect simply "remembers"
     server's address+port# */
  err = connect(sd, (struct sockaddr *) &server,
                sizeof(struct sockaddr_in));

  return(sd);
}

/*  End UDP  */


/////////////////////////
//         TCP         //
/////////////////////////

int TCP_socks_servinit(char *progname, struct sockaddr_in *self, char *sname, int reuse) {
  int sd;
  struct hostent *sp;
  unsigned int localaddr;
  u_short localport;
  int err, len;

  /* create a TCP socket */
  sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  
  /* initialize self */
  self->sin_family = AF_INET;
  self->sin_addr.s_addr = INADDR_ANY;
  localport = self->sin_port = htons(65000); // set by caller

  /* 
   * reuse local port so that the call to bind with the same 
   * ephemeral port, doesn't complain of "Address already 
   * in use" or "Can't assign requested address." 
  */
  err = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(int));
#ifdef SO_REUSEPORT
  err = setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, (char *) &reuse, sizeof(int));
#endif

  /* bind address to socket */
  err = bind(sd, (struct sockaddr *) self, sizeof(struct sockaddr_in));

  /* listen on socket */
  err = listen(sd, PHONE_QLEN);

  if (!localport) {
    /*
     * if self->sin_port was 0, obtain the ephemeral port assigned to
     * socket and store it in the member variable "self".
     */
    len = sizeof(struct sockaddr_in);
    err = getsockname(sd, (struct sockaddr *) self, (socklen_t *) &len);
  }

  /* Find out the FQDN of the current host and store it in the
     variable "sname".  gethostname() is usually sufficient. */
  err = gethostname(sname, MAX_FNAME);
  
  /* Check if the hostname is a valid FQDN or a local alias.
     If local alias, set sname to "localhost" and addr to 127.0.0.1 */
  sp = gethostbyname(sname);
  if (sp) {
    memcpy(&self->sin_addr, sp->h_addr, (size_t)sp->h_length);
  } else {
    localaddr = (unsigned int) inet_addr("127.0.0.1");
    memcpy(&self->sin_addr, (char *) &localaddr, sizeof(unsigned int));
    strcpy(sname, "localhost");
  }
    
  /* inform user which port this server is listening on */
  fprintf(stderr, "%s address is %s:%d\n",
          progname, sname, ntohs(self->sin_port));
    
  return sd;
}

int TCP_socks_clntinit(struct sockaddr_in *server, char *sname, int reuse, struct sockaddr_in *self) {
  int sd;
  int err;
  struct hostent *sp;

  /* create a new TCP socket */
  sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  /* reuse local port so that the call to bind in socks_servinit(), 
     to bind to the same ephemeral port, doesn't complain of 
     "Address already in use" or "Can't assign requested address." */
  err = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(int));
#ifdef SO_REUSEPORT
  err = setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, (char *) &reuse, sizeof(int));
#endif

  if (self) {
    /* set the outgoing IP address and port number
     * to be those stored in "self".  Used in p2p to ID each peer
     * with a unique addr:port pair.
     */
    err = bind(sd, (struct sockaddr *) self, sizeof(struct sockaddr_in));
  }
 
  /* initialize the socket address with server's address and port
     number.  If provided server's address is NULL, obtain the
     server's address from sname. */
  server->sin_family = AF_INET;
  if (!server->sin_addr.s_addr) {
    sp = gethostbyname(sname);
    memcpy(&server->sin_addr, sp->h_addr, (size_t)sp->h_length);
  }

  /* connect to server.  If connect() fails, close sd and return
     SOCKS_UNINIT_SD */
  err = connect(sd, (struct sockaddr *) server, sizeof(struct sockaddr_in));
  if (err) {
    socks_close(sd);
    return(SOCKS_UNINIT_SD);
  }

  return(sd);
}

int TCP_socks_accept(int sd) {
  int td;
  int len;
  struct sockaddr_in sockaddr;
  struct sockaddr_in *client;
  struct hostent *cp;

  client = &sockaddr;

  /* Accept the new connection.  Use the variable "td" to hold the new
   * connected socket.  Use the local variable "client" in the call to
   * accept() to hold the connected client's sockaddr_in info.
  */
  len = sizeof(struct sockaddr_in);
  td = accept(sd, (struct sockaddr *) client, (socklen_t *) &len);

  /* inform user of connection */
  cp = gethostbyaddr((char *) &client->sin_addr,
                     sizeof(struct in_addr), AF_INET);
  fprintf(stderr, "Connected from client %s:%d\n",
          ((cp && cp->h_name) ? cp->h_name : inet_ntoa(client->sin_addr)),
          ntohs(client->sin_port));

  return(td);
}

/*  End TCP  */

void socks_close(int td) {
  close(td);
  return;
}

