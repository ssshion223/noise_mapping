#pragma once

#include <rclcpp/rclcpp.hpp>
#include "microphone.hpp"
#include "ringbuffer.hpp"
#include "audio_process.hpp"
#include "audio_interfaces/msg/audio_msg.hpp"
class AudioNode : public rclcpp::Node {
public:
  AudioNode();
  ~AudioNode();

private:
  void audio_cb();
private:
  RingBuffer ringbuffer_;
  MicroPhone microphone_;
  AudioProcess audio_process_;

  rclcpp::Publisher<audio_interfaces::msg::AudioMsg>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};