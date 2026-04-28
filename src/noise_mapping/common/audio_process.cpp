#include "audio_process.hpp"

AudioProcess::AudioProcess(RingBuffer &rb): ringbuffer_(rb), running_(false){}
AudioProcess::~AudioProcess(){}

float AudioProcess::computeEnergy(const float *input, size_t n){
  float energy = 0;
  for (size_t i = 0; i < n; i++) {
    energy += input[i]*input[i];
  }
  float rms = sqrt(energy / n);
  const float eps = 1e-10f;
  float db = 20.0f * log10(rms + eps); // DBFS
  return db;
}

bool AudioProcess::start(){
  if (running_) return false;
  running_ = true;
  thread_ = std::thread(&AudioProcess::run, this);
}
bool AudioProcess::stop(){
  running_ = false;
  if (thread_.joinable()) thread_.join();
}

void AudioProcess::run(){
  std::vector<float> frame(1024);
  while (running_) {
    size_t n = ringbuffer_.pop(frame, 1024);
    if (n == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }
    AudioFrame af_temp;
    af_temp.energy = computeEnergy(frame.data(), n);
    audio_frame_.store(af_temp);
  }
}
