#pragma once

#include <grid_map_core/GridMap.hpp>
#include <grid_map_core/iterators/GridMapIterator.hpp>
#include <grid_map_ros/grid_map_ros.hpp>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>

class GridMapNode :public rclcpp::Node {
public:
  GridMapNode();
  ~GridMapNode();
private:  //private function
  void grid_map_cb();
private:  //private member
  grid_map::GridMap map_({"energy"});
  rclcpp::Publisher<grid_map_msgs::msg::GridMap>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};