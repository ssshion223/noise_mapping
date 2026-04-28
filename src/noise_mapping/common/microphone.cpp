#include "microphone.hpp"

MicroPhone::MicroPhone(RingBuffer &rb): ringbuffer_(rb), framecount_(256) {}
MicroPhone::~MicroPhone(){}

bool MicroPhone::init(){
  PaError err = Pa_Initialize();
  if (err != paNoError) {
    std::cerr << "PortAudio Initialization Failed" << std::endl;
    return false;
  }
  // device_idx_ = Pa_GetDefaultInputDevice();
  device_idx_ = findDeviceByName("Yundea");
  if (device_idx_ == paNoDevice) {
    std::cerr << "Default Device Not Found" << std::endl;
    return false;
  }
  device_info_ = Pa_GetDeviceInfo(device_idx_);
  if (!device_info_ || device_info_->maxInputChannels <= 0) {
    std::cerr << "Invalid Input Device" << std::endl;
    return false;
  }
  int numDevices = Pa_GetDeviceCount();

  // for (int i = 0; i < numDevices; i++) {  //for debug

  //     const PaDeviceInfo* info =
  //         Pa_GetDeviceInfo(i);

  //     std::cout
  //         << i
  //         << " : "
  //         << info->name
  //         << std::endl;
  // }
  std::cout << device_info_->name << std::endl;
  smaplerate_ = device_info_->defaultSampleRate;
  return true;
}

bool MicroPhone::start(){
  if (running_) return true;
  PaStreamParameters inputParams {};
  inputParams.device = device_idx_;
  inputParams.channelCount = 1;
  inputParams.sampleFormat = paFloat32;
  inputParams.suggestedLatency = device_info_->defaultLowInputLatency;
  inputParams.hostApiSpecificStreamInfo = nullptr;
  PaError err = Pa_OpenStream(
    &stream_,
    &inputParams,
    nullptr,
    smaplerate_,
    framecount_,
    paNoFlag,
    &MicroPhone::paCallback,
    this
  );
  if (err != paNoError || !stream_) {
    std::cerr << "Ftart Ftream Failed" << std::endl;
    running_ = false;
    return false;
  }
  err = Pa_StartStream(stream_);
  if (err != paNoError) {
    std::cerr << "Pa_StartStream failed: " << err << std::endl;
    return false;
  running_ = true;
  return true;

}
}
int MicroPhone::paCallback(    
    const void *input, 
    void *output, 
    unsigned long frameCount, 
    const PaStreamCallbackTimeInfo *timeInfo, 
    PaStreamCallbackFlags statusFlags, 
    void *userData
  ){
    MicroPhone *self = static_cast<MicroPhone*>(userData);
    const float *_input = static_cast<const float*>(input);
    if (!_input) {
      return paContinue;
    }
    std::cout << _input[0] << std::endl;
    self->onAudio(_input, frameCount);
    return paContinue;
}

void MicroPhone::onAudio(const float *input, unsigned long framecount){
  // computeEnergy(input, framecount);
  ringbuffer_.push(input, framecount);
}
// float MicroPhone::computeEnergy(const float *input, unsigned long framecount){
//   float energy = 0;
//   for (unsigned long i = 0; i < framecount; i++) {
//     float x = input[i];
//     energy += x*x;
//   }
//   return energy;
// }
void MicroPhone::stop(){
  if (!stream_) return;
  if (Pa_IsStreamActive(stream_) == 1) {
      Pa_StopStream(stream_);
  }
  Pa_CloseStream(stream_);
  stream_ = nullptr;
  running_ = false;
}

int MicroPhone::findDeviceByName(const std::string &keyword){
    int numDevices = Pa_GetDeviceCount();
    for (int i = 0; i < numDevices; i++) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (!info) continue;
        std::string name = info->name ? info->name : "";
        if (name.find(keyword) != std::string::npos) {
          if (info->maxInputChannels > 0) {
            return i;
          }
        }
    }
    return -1;
}
