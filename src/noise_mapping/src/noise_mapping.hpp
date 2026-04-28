#pragma once

#include <grid_map_core/GridMap.hpp>
#include <grid_map_core/iterators/GridMapIterator.hpp>
#include <grid_map_ros/grid_map_ros.hpp>


class GridMapNode :public rclcpp::Node {
public:
  GridMapNode();
  ~GridMapNode();
private:  //private function

private:  //private member
  rclcpp::Publisher<grid_map_msgs::msg::GridMap>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};