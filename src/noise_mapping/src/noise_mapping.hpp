#pragma once
#include <mutex>
// #include <grid_map_core/GridMap.hpp>
// #include <grid_map_core/iterators/GridMapIterator.hpp>
#include <grid_map_ros/grid_map_ros.hpp>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include "audio_interfaces/msg/audio_msg.hpp"
class GridMapNode :public rclcpp::Node {
public:
  GridMapNode();
  ~GridMapNode();
private:  //private function
  void initialGridMap();
  void audio_listen_cb(const audio_interfaces::msg::AudioMsg::SharedPtr msg);
  void updategrid_cb();
  void publishGridMap_cb();
private:  //private member
  grid_map::GridMap map_;
  rclcpp::Subscription<audio_interfaces::msg::AudioMsg>::SharedPtr audio_listener_;
  std::mutex audio_mutex_;
  audio_interfaces::msg::AudioMsg::SharedPtr latest_audio_;
  rclcpp::Publisher<grid_map_msgs::msg::GridMap>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_update_;
  rclcpp::TimerBase::SharedPtr timer_publish_;
  std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
};