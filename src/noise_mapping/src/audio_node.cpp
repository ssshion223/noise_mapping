#include "audio_node.hpp"

AudioNode::AudioNode() : Node("audio_node"), ringbuffer_(1024), 
microphone_(ringbuffer_), audio_process_(ringbuffer_){
  pub_ = this->create_publisher<audio_interfaces::msg::AudioMsg>("/audio_msg", 10);
  timer_ = this->create_wall_timer(std::chrono::milliseconds(10), std::bind(&AudioNode::audio_cb, this));
  microphone_.init();
  microphone_.start();
  audio_process_.start();
}

AudioNode::~AudioNode(){
  audio_process_.stop();
  microphone_.stop();
}

void AudioNode::audio_cb(){
  audio_interfaces::msg::AudioMsg msg;
  AudioFrame af = audio_process_.audio_frame_.load(std::memory_order_acquire);
  msg.dbfs = af.energy;
  msg.header.stamp = this->now();
  msg.header.frame_id = "base_link";
  pub_->publish(msg);
}

int main(int argc, char **argv){
  rclcpp::init(argc, argv);
  auto node = std::make_shared<AudioNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}