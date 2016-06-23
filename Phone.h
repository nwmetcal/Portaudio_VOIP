#ifndef PHONE_H_
#define PHONE_H_

#include "socks_util.h"
#include "pa_utility.h"

#include <netinet/in.h>
#include <deque>
#include <array>

// Size of server listen queue
#define PHONE_QLEN        10
// Minimum number of samples for playback in tcp mode
#define DATA_BUFFER_SIZE  0
#define SERVER_PORT       65000
// Size of receive buffer
#define MAX_RECV_BUF      10*sizeof(micmsg_t)
// Number of seconds to sleep when data is not ready
#define TCP_SLEEP_TIME    1 

class Phone {
public:
// Constructor parses arguments and sets up sockets
  Phone(int argc, char *argv[]);

//  Parse command-line arguments
  int parse_args(int argc, char *argv[]);

// initializes audio streams and runs as either server or receiver
  void run();

private:

  bool is_server;
  int client_sd;
  int sd;    // Socket to receive data
  struct sockaddr_in self;
  struct sockaddr_in client;
  char sname[MAX_FNAME];
  unsigned short port;

  PortAudio pa;

  PaStream *stream1;
  PaStream *stream2;

  std::deque<std::array<float, sample_size>> mic_data;
 
/*
  Print a list of available devices.
  Prompt user to select input and output devices.
  Initialize and start streams.
*/
  void init_streams();

/*
  Wait for start request from client.
  Once request is received, spawn threads for recording/sending data
*/
  void server_func();

//  Enter loop to record data and push to mic_data deque
  static PaError read_mics(Phone *phone_in);

//  Enter loop which sends data at head of mic_data deque then pops it
  static void process_audio(Phone *phone_in);

//  Send start request to server, then spawn receiving/playback threads
  void receiver_func();

//  Receive data from server and push to back of mic_data deque
  static void sock_recv(Phone *phone_in);

/*
  Playback data at front of mic_data deque then pop it
  If data is not ready and in tcp mode, sleep for TCP_SLEEP_TIME seconds
*/
  static void playback(Phone *phone_in);
};

#endif /* PHONE_H_ */
