#include "serial_audio.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <stdexcept>

namespace {
constexpr uint8_t kFrameHead0 = 0xE1;
constexpr uint8_t kFrameHead1 = 0xDB;
constexpr float kDbfsScale = 100.0f;

float decode_dbfs(uint8_t dbfs_low, uint8_t dbfs_high) {
  const auto raw = static_cast<int16_t>(
      static_cast<uint16_t>(dbfs_low) |
      (static_cast<uint16_t>(dbfs_high) << 8));
  return static_cast<float>(raw) / kDbfsScale;
}
}  // namespace

SerialAudioNode::SerialAudioNode()
    : Node("serial_audio_node"),
      port_(this->declare_parameter<std::string>("port", "/dev/ttyUSB0")),
      baud_rate_(this->declare_parameter<int>("baud_rate", 115200)),
      running_(false),
      has_data_(false),
      latest_dbfs_(0.0f),
      latest_norm_dbfs_(0.0f) {
  pub_ = this->create_publisher<audio_interfaces::msg::AudioMsg>("/audio_msg", 10);
       
  if (!open_serial()) {
    throw std::runtime_error("failed to open serial port");
  }

  running_ = true;
  read_thread_ = std::thread(&SerialAudioNode::read_loop, this);
  timer_ = this->create_wall_timer(
      std::chrono::milliseconds(10),
      std::bind(&SerialAudioNode::publish_cb, this));
}

SerialAudioNode::~SerialAudioNode() {
  running_ = false;
  if (read_thread_.joinable()) {
    read_thread_.join();
  }
  close_serial();
}

bool SerialAudioNode::open_serial() {
  try {
    const auto baud = baud_to_constant(baud_rate_);
    if (baud == LibSerial::BaudRate::BAUD_INVALID) {
      RCLCPP_ERROR(this->get_logger(), "Unsupported baud_rate: %d", baud_rate_);
      return false;
    }

    serial_.Open(port_);
    serial_.SetBaudRate(baud);
    serial_.SetCharacterSize(LibSerial::CharacterSize::CHAR_SIZE_8);
    serial_.SetParity(LibSerial::Parity::PARITY_NONE);
    serial_.SetStopBits(LibSerial::StopBits::STOP_BITS_1);
    serial_.SetFlowControl(LibSerial::FlowControl::FLOW_CONTROL_NONE);
    serial_.FlushIOBuffers();
  } catch (const std::exception &e) {
    RCLCPP_ERROR(
        this->get_logger(), "Open serial port %s failed: %s",
        port_.c_str(), e.what());
    return false;
  }

  RCLCPP_INFO(
      this->get_logger(), "Serial audio opened: %s, baud_rate=%d",
      port_.c_str(), baud_rate_);
  return true;
}

void SerialAudioNode::close_serial() {
  try {
    if (serial_.IsOpen()) {
      serial_.Close();
    }
  } catch (const std::exception &e) {
    RCLCPP_WARN(this->get_logger(), "Close serial failed: %s", e.what());
  }
}

void SerialAudioNode::read_loop() {
  enum class ParseState {
    WaitHead0,
    WaitHead1,
    SeqLow,
    SeqHigh,
    DbfsLow,
    DbfsHigh,
  };

  ParseState state = ParseState::WaitHead0;
  uint8_t dbfs_low = 0;

  while (running_) {
    unsigned char byte = 0;
    try {
      serial_.ReadByte(byte, 100);
    } catch (const LibSerial::ReadTimeout &) {
      continue;
    } catch (const std::exception &e) {
      RCLCPP_WARN_THROTTLE(
          this->get_logger(), *this->get_clock(), 1000,
          "Read serial failed: %s", e.what());
      continue;
    }

    switch (state) {
      case ParseState::WaitHead0:
        if (byte == kFrameHead0) {
          state = ParseState::WaitHead1;
        }
        break;

      case ParseState::WaitHead1:
        if (byte == kFrameHead1) {
          state = ParseState::SeqLow;
        } else {
          state = byte == kFrameHead0 ? ParseState::WaitHead1
                                      : ParseState::WaitHead0;
        }
        break;

      case ParseState::SeqLow:
        state = ParseState::SeqHigh;
        break;

      case ParseState::SeqHigh:
        state = ParseState::DbfsLow;
        break;

      case ParseState::DbfsLow:
        dbfs_low = byte;
        state = ParseState::DbfsHigh;
        break;

      case ParseState::DbfsHigh: {
        const float dbfs = decode_dbfs(dbfs_low, byte);
        {
          std::lock_guard<std::mutex> lock(data_mutex_);
          latest_dbfs_ = dbfs;
          latest_norm_dbfs_ = normalize_dbfs(dbfs);
          has_data_ = true;
        }
        state = ParseState::WaitHead0;
        break;
      }
    }
  }
}

void SerialAudioNode::publish_cb() {
  float dbfs = 0.0f;
  float norm_dbfs = 0.0f;
  {
    std::lock_guard<std::mutex> lock(data_mutex_);
    if (!has_data_) {
      return;
    }
    dbfs = latest_dbfs_;
    norm_dbfs = latest_norm_dbfs_;
  }

  audio_interfaces::msg::AudioMsg msg;
  msg.dbfs = dbfs;
  msg.norm_dbfs = norm_dbfs;
  msg.header.stamp = this->now();
  msg.header.frame_id = "base_link";
  pub_->publish(msg);
}

float SerialAudioNode::normalize_dbfs(float dbfs) const {
  return std::clamp((dbfs + 60.0f) / 10.0f, 0.0f, 1.0f);
}

LibSerial::BaudRate SerialAudioNode::baud_to_constant(int baud_rate) const {
  switch (baud_rate) {
    case 9600:
      return LibSerial::BaudRate::BAUD_9600;
    case 19200:
      return LibSerial::BaudRate::BAUD_19200;
    case 38400:
      return LibSerial::BaudRate::BAUD_38400;
    case 57600:
      return LibSerial::BaudRate::BAUD_57600;
    case 115200:
      return LibSerial::BaudRate::BAUD_115200;
    case 230400:
      return LibSerial::BaudRate::BAUD_230400;
    default:
      return LibSerial::BaudRate::BAUD_INVALID;
  }
}

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<SerialAudioNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
