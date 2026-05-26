#pragma once
#include <mutex>
// #include <grid_map_core/GridMap.hpp>
// #include <grid_map_core/iterators/GridMapIterator.hpp>
#include <grid_map_ros/grid_map_ros.hpp>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <tf2/utils.h>
#include "audio_interfaces/msg/audio_msg.hpp"
class GridMapNode : public rclcpp::Node {
public:
  GridMapNode();
  ~GridMapNode();
private:  //private function
  void initialGridMap();
  void audio_listen_cb(const audio_interfaces::msg::AudioMsg::SharedPtr msg);
  void local_update_cb();
  void global_update_cb();
  void global_pub_cb();
  void compute_kernel(const int &R, std::vector<std::vector<float>> &k, const float &sigma);
private:  //private member
  int global_weight_ = 80;
  int global_height_ = 80;
  float resolution_ = 0.4;
  int R_ = 8; //numbers of expanding
  std::vector<std::vector<float>> Gaussian_kernel_;
  float sigma_ = 1.2;
  float alpha_ = 0.9;
  geometry_msgs::msg::TransformStamped tf_;
  grid_map::GridMap local_map_;
  grid_map::GridMap global_map_;
  rclcpp::Subscription<audio_interfaces::msg::AudioMsg>::SharedPtr audio_listener_;
  std::mutex audio_mutex_;
  std::mutex local_mutex_;
  std::mutex tf_mutex_;
  std::mutex global_mutex_;
  audio_interfaces::msg::AudioMsg::SharedPtr latest_audio_;
  rclcpp::Publisher<grid_map_msgs::msg::GridMap>::SharedPtr local_pub_;
  rclcpp::Publisher<grid_map_msgs::msg::GridMap>::SharedPtr global_pub_;
  rclcpp::TimerBase::SharedPtr local_update_timer_;
  rclcpp::TimerBase::SharedPtr global_update_timer_;
  rclcpp::TimerBase::SharedPtr global_pub_timer_;
  std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
};