#include "noise_mapping.hpp"


GridMapNode::GridMapNode(): Node("grid_map_node") {

  pub_ = this->create_publisher<grid_map_msgs::msg::GridMap>("/grid_map", 10);
  timer_ = this->create_wall_timer(std::chrono::milliseconds(1000), std::bind(&GridMapNode::grid_map_cb, this));
}

GridMapNode::~GridMapNode(){}


void GridMapNode::grid_map_cb(){
  grid_map_msgs::msg::GridMap msg;
  msg.
}