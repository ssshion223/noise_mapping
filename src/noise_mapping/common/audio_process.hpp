#pragma once

#include <vector>
#include <iostream>
#include <cmath>
#include <thread>
#include <atomic>
#include "ringbuffer.hpp"
struct AudioFrame {
  float energy;
  float norm_energy;
};
class AudioProcess {
public:
  explicit AudioProcess(RingBuffer &rb);
  ~AudioProcess();

public: //public member
  std::atomic<AudioFrame> audio_frame_;
public: //public function
  float computeEnergy(const float *input, size_t n);
  bool start();
  bool stop();
private: //private function
  void run();
private: //private member

  std::thread thread_;
  std::atomic<bool> running_;
  RingBuffer &ringbuffer_;
};