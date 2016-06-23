#include "pa_utility.h"

#include <iostream>
#include <cstring>
#include <string>
#include <cmath>

using std::cout; using std::cerr; using std::endl;

bool PortAudio::check_and_print_error(const PaError err, const char * const txt) const {
  if (err != paNoError) {
    cerr << "Error " << txt << ": " << Pa_GetErrorText(err) << endl;
    return false;
  }
  return true;
}

PortAudio::PortAudio() {
  Pa_Initialize();
  sample_rate_ = 44100;
  frames_per_buffer_ = paFramesPerBufferUnspecified;
}

PortAudio::~PortAudio() {
  Pa_Terminate();
}

const char * PortAudio::GetVersionText() {
  return Pa_GetVersionText();
}

void PortAudio::SetFramesPerBuffer(int fpb) {
  if (fpb < 0) {
    frames_per_buffer_ = paFramesPerBufferUnspecified;
  }
  else {
    frames_per_buffer_ = (size_t)fpb;
  }
}

bool PortAudio::StartStream(PaStream *stream) {
  PaError err = Pa_StartStream(stream);
  return check_and_print_error(err, "Starting Stream");
}

bool PortAudio::AbortStream(PaStream *stream) {
  PaError err = Pa_AbortStream(stream);
  return check_and_print_error(err, "Aborting Stream");
}

PaStream* PortAudio::OpenStream(int device, PaStreamCallback callback, void *userdata) {
  PaStream *stream;

  PaStreamParameters input_param;
  memset(&input_param, 0, sizeof(input_param));

  input_param.channelCount = 1;//Pa_GetDeviceInfo(device)->maxInputChannels;
  input_param.device = device;
  input_param.hostApiSpecificStreamInfo = NULL;
  input_param.sampleFormat = paFloat32;
  input_param.suggestedLatency = Pa_GetDeviceInfo(device)->defaultHighInputLatency;
  input_param.hostApiSpecificStreamInfo = NULL;
  
  PaError err = Pa_OpenStream(&stream,
                              &input_param,
                              NULL,
                              sample_rate_,
                              frames_per_buffer_,
                              paNoFlag,
                              callback,
                              userdata);
  bool res = check_and_print_error(err, "Opening Stream");
  if (res) {
    return stream;
  }
  else {
    return NULL;
  }

}

PaStream* PortAudio::OpenOutputStream(int device, PaStreamCallback callback, void *userdata) {
  PaStream *stream;

  PaStreamParameters output_param;
  memset(&output_param, 0, sizeof(output_param));

  output_param.channelCount = 1;
  output_param.device = device;
  output_param.hostApiSpecificStreamInfo = NULL;
  output_param.sampleFormat = paFloat32;
  output_param.suggestedLatency = Pa_GetDeviceInfo(device)->defaultHighInputLatency;
  output_param.hostApiSpecificStreamInfo = NULL;
  
  PaError err = Pa_OpenStream(&stream,
                              NULL,
                              &output_param,
                              sample_rate_,
                              frames_per_buffer_,
                              paNoFlag,
                              callback,
                              userdata);
  bool res = check_and_print_error(err, "Opening Stream");
  if (res) {
    return stream;
  } 
  else {
    return NULL;
  }

}

int PortAudio::PrintDevices() {

  int num_devices = Pa_GetDeviceCount();
  
  if (num_devices < 0) {
    cerr << "Returned negative count of devices available: " << num_devices << endl;
    cerr << Pa_GetErrorText(num_devices) << endl;
    return num_devices;
  } 
  else {
    cout << "***Returned " << num_devices << " Devices***" << endl;
  }
  
  for (int i = 0; i < num_devices; ++i) {
    const PaDeviceInfo *device_info = Pa_GetDeviceInfo(i);

    cout << "Device[" << i << "]: " << device_info->name << endl;
  }
  return num_devices;

}
