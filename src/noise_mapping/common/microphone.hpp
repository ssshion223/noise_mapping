#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <portaudio.h>
#include "ringbuffer.hpp"
class MicroPhone {
public:
  MicroPhone(RingBuffer &rb);
  ~MicroPhone();

  bool init();
  bool start();
  void stop();
private: //private function
  int findDeviceByName(const std::string &keyword);
  void onAudio(const float *input, unsigned long framecount);
  // float computeEnergy(const float *input, unsigned long framecount);
  static int paCallback(
    const void *input, 
    void *output, 
    unsigned long frameCount, 
    const PaStreamCallbackTimeInfo *timeInfo, 
    PaStreamCallbackFlags statusFlags, 
    void *userData
  );
private: //private member
  RingBuffer &ringbuffer_;
  double smaplerate_;
  unsigned long framecount_;
  bool running_;
  bool initialized_ = false;
  bool streaming_ = false;
  PaDeviceIndex device_idx_;
  const PaDeviceInfo *device_info_;
  PaStream *stream_ = nullptr;
};

