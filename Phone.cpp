#include "Phone.h"

#include "socks_util.h"

#include <iostream>
#include <cstring>
#include <thread>
#include <mutex>

#include <netdb.h>         // gethostbyname()
#include <netinet/in.h>    // struct in_addr
#include <arpa/inet.h>     // htons()
#include <sys/types.h>     // u_short
#include <sys/socket.h>    // socket API
#include <sys/ioctl.h>     // ioctl(), FIONBIO
#include <unistd.h>
#include <getopt.h>

using std::mutex; using std::thread; using std::array;
using std::cout; using std::cerr; using std::cin; using std::endl; using std::flush;


// Helper functions
void print_usage();

// lock for audio data
mutex data_mutex;
// flag for enabling tcp
bool tcp_mode = false;

////////////////////////////
// Begin Public Functions //
////////////////////////////

Phone::Phone(int argc, char *argv[]) : sd(0), stream1(NULL), stream2(NULL) {
  parse_args(argc, argv);

  if (is_server) {
    if (tcp_mode) {
    sd = TCP_socks_servinit((char *) "phone", &self, sname, 1);
    }
    else {
    sd = UDP_socks_servinit((char *) "phone", &self, sname);
    }
  }
  else {
    self.sin_addr.s_addr = (in_addr_t)NULL;
    self.sin_port = (in_port_t)port;
    if (tcp_mode) {
      client_sd = TCP_socks_clntinit(&self, sname, 0, NULL);    
    }
    else {
      client_sd = UDP_socks_clntinit(sname, port, MAX_RECV_BUF);    
    }
  }
} 

int Phone::parse_args(int argc, char *argv[]) {
  char c;
  extern char *optarg;
  bool role_set = false;
  bool server_set = false;
  bool mode_set = false;
  char *p;

  while ((c = (char)getopt(argc, argv, "r:s:m:h")) != EOF) {
    switch (c) {
      case 'r': {
        if (!role_set) {
          is_server = (atoi(optarg) == 0);
          role_set = true;
        }
        else {
          cout << "Error: Must specify only one of -r" << endl;
          exit(1);
        }
        break;
      }
      case 's': {
        for (p = optarg+strlen(optarg)-1;  // point to last character of
                                           // addr:port arg
             p != optarg && *p != ':';
                                           // search for ':' separating
                                           // addr from port
             p--);
  
        *p++ = '\0';
        port = htons((u_short) atoi(p)); // always stored in network byte order
  
        strcpy(sname,optarg);
        server_set = true;
        break;
      }
      case 'm': {
        if (mode_set) {
          cout << "Error: Must specify a single mode <TCP/UDP>" << endl;
          exit(1);
        }
        if (!strcmp(optarg, "TCP")) {
          tcp_mode = true;
          mode_set = true;
        }
        else if (!strcmp(optarg, "UDP")) {
          tcp_mode = false;
          mode_set = true;
        }
      }
      case 'h': {
        print_usage();
        exit(0);
      }
      default: {
        print_usage();
        exit(1);
        break;
      }
    }
  }

  if (!role_set) {
    cout << "Error: Must specify -r [0/1]" << endl;
    exit(1);
  }
  if (!is_server && !server_set) {
    cout << "Error: Set as receiver, but no server address provided" << endl;
    exit(1);
  }

  return (0);
}

void Phone::run() {
  init_streams();
  if (is_server) {
    server_func();
  }
  else {
    receiver_func();
  }
}

/* End Public Functions */

/////////////////////////////
// Begin Private Functions //
/////////////////////////////

void Phone::init_streams() {
  if (!PortAudio::PrintDevices()) {
    cerr << "Error: No audio devices found\n" << flush;
    exit(1);
  }

  int dev1;
  cout << "Enter input device: ";
  cin >> dev1;
  int dev2;
  cout << "Enter output device: ";
  cin >> dev2;
  
  stream1 = pa.OpenStream( dev1 );
  cout << "SDFS" << endl;
  stream2 = pa.OpenOutputStream( dev2 );

  pa.StartStream( stream1 );
  pa.StartStream( stream2 );
}

////////////////////////////////////////////////
/*                                            */
/*              Record/Transmit               */
/*                                            */
////////////////////////////////////////////////


void Phone::server_func() {
  // Wait for client
  ssize_t bytes;  // stores the return value of recvfrom()
  socklen_t len;
  request_t rqst;

  if (tcp_mode) {
    client_sd = TCP_socks_accept(sd);
  }

  len = sizeof(struct sockaddr_in);
  cout << "about to call recv" << endl;

  if (tcp_mode) {
    bytes = recv(client_sd, (void *)&rqst, sizeof(request_t), MSG_WAITALL);
  }
  else {
    bytes = recvfrom(sd, (void *)&rqst, sizeof(request_t), MSG_WAITALL,
                     (struct sockaddr *) &client, &len);
  }

  if (rqst.snd_rqst) {
    cout << "Entered record loop" << endl;

    // Spawn mic thread and sender thread
  
    thread retrieve_input(Phone::read_mics, this);
    thread process_input(Phone::process_audio, this);
    retrieve_input.detach();
    process_input.join();  
  }
}

PaError Phone::read_mics(Phone *phone_in) {
  PaError err;
  array<float,sample_size> arr;

  while(true) {
    err = Pa_ReadStream( phone_in->stream1, arr.data(), sample_size );
    if (err != paNoError) {
      cerr << "Error: " << Pa_GetErrorText( err ) << endl;
    }
    data_mutex.lock();
    phone_in->mic_data.push_back(arr);
    data_mutex.unlock();
  }
}

void Phone::process_audio(Phone *phone_in) {
  ssize_t ret;
// send loop
  while (true) {
    data_mutex.lock();
    if (phone_in->mic_data.empty()) {
      data_mutex.unlock();
      usleep(10000);
      continue;
    }
    data_mutex.unlock();

    if (tcp_mode) {
      ret = send(phone_in->client_sd, 
                 (void *)phone_in->mic_data.front().data(), 
                 sample_size*sizeof(float), 
                 0);
    }
    else {
      ret = sendto(phone_in->sd, (void *)phone_in->mic_data.front().data(), sample_size*sizeof(float),
                   0, (struct sockaddr*)&(phone_in->client), (socklen_t)sizeof(phone_in->client));
    }

    data_mutex.lock();
    if (ret < 0) {
      perror("Error in Sendto");
    }
    phone_in->mic_data.pop_front();
    data_mutex.unlock();
  }
}

////////////////////////////////////////////////
/*                                            */
/*             Receive/Playback               */
/*                                            */
////////////////////////////////////////////////

void Phone::receiver_func() {
  request_t rqst;
  rqst.snd_rqst = true;

  ssize_t err = send(client_sd, (void *)&rqst, sizeof(request_t), 0);
  cout << err << endl;
  if (err < 0) {
    perror("SENDTO: ");
  }

  thread sock_recv_thread(Phone::sock_recv, this);
  thread playback_thread(Phone::playback, this);
  
  sock_recv_thread.detach();
  playback_thread.join();
}

void Phone::sock_recv(Phone *phone_in) {
  ssize_t ret;
  array<float,sample_size> arr;
  while(true) {
    ret = recv(phone_in->client_sd, (void *)arr.data(), sample_size*sizeof(float), MSG_WAITALL);
    if (ret < 0) {
      perror("Erro in Recv");
    }
    data_mutex.lock();
    phone_in->mic_data.push_back(arr);
    data_mutex.unlock();
  }
}

void Phone::playback(Phone *phone_in) {
  bool data_ready = false;
  while (true) {

    if (!data_ready && tcp_mode) {
      cout << "Data size: " << phone_in->mic_data.size() << "\n";
      cout << "Sleeping\n";
      sleep(TCP_SLEEP_TIME);
    }
    data_mutex.lock();
    if (phone_in->mic_data.empty()) {
      data_ready = false;
    }
    else if (phone_in->mic_data.size() >= DATA_BUFFER_SIZE) {
      data_ready = true;
    }
    data_mutex.unlock();

    if (data_ready) {
      Pa_WriteStream(phone_in->stream2, phone_in->mic_data.front().data(), sample_size);
      
      data_mutex.lock();
      phone_in->mic_data.pop_front();
      data_mutex.unlock();
    }
  }
}

/* End Private Functions */

////////////////////////////
// Begin Helper Functions //
////////////////////////////

void print_usage() {
  cout << "Usage:\n\t./exec -r 0/1 <-m UDP/TCP> -s hostname:port\n";
  cout << "Options:\n";
  cout << "-r           \t0 - Program is server\n";
  cout << "             \t1 - Program is client\n";
  cout << "-m (Optional)\tUDP - Operate using UDP sockets\n";
  cout << "             \tTCP - Operate using TCP sockets\n";
  cout << "             \tUses UDP by default\n";
  cout << "-s           \thostname:port specifies the which server and port to connect to. If not client, option is ignored\n";
}

/* End Helper Functions */
