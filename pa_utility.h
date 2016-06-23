#ifndef PA_UTILITY_H_
#define PA_UTILITY_H_

#include "portaudio.h"
#include <cstdlib>

constexpr double sample_rate = 44100;
constexpr size_t sample_size = 256; 

struct micmsg_t {
  float data[sample_size];
};

struct request_t {
  bool snd_rqst;
};

class PortAudio {
  
private:
// Gets error text and prints
  bool check_and_print_error(const PaError err, const char * const txt) const;

  int sample_rate_;
  size_t frames_per_buffer_;

public:
  
// Initialize portaudio and set sample_rate_ and frames_per_buffer_  
  PortAudio();
  
// Closes portaudio
  ~PortAudio();
  
// Returns current version of portaudio
  const char *GetVersionText();

// Set frames per buffer, udefined if fpb is negative
  void SetFramesPerBuffer(int fpb);

  bool StartStream(PaStream *stream);
  
  bool AbortStream(PaStream *stream);

// Opens a mono input stream
  PaStream *OpenStream(int device,
                       PaStreamCallback callback = NULL,
                       void *userdata = NULL);

// Opens a mono output stream
  PaStream *OpenOutputStream(int device,
                             PaStreamCallback callback = NULL,
                             void *userdata = NULL);


// Print list of available devices
  static int PrintDevices();

};
#endif /* PA_UTILITY_H_ */
