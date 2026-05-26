#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include <rclcpp/rclcpp.hpp>
#include <libserial/SerialPort.h>

#include "audio_interfaces/msg/audio_msg.hpp"

class SerialAudioNode : public rclcpp::Node {
public:
  SerialAudioNode();
  ~SerialAudioNode() override;

private:
  void publish_cb();
  void read_loop();
  bool open_serial();
  void close_serial();
  float normalize_dbfs(float dbfs) const;
  LibSerial::BaudRate baud_to_constant(int baud_rate) const;

  std::string port_;
  int baud_rate_;
  LibSerial::SerialPort serial_;
  std::atomic<bool> running_;
  std::thread read_thread_;

  std::mutex data_mutex_;
  bool has_data_;
  float latest_dbfs_;
  float latest_norm_dbfs_;

  rclcpp::Publisher<audio_interfaces::msg::AudioMsg>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};
