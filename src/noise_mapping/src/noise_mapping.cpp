#include "noise_mapping.hpp"


GridMapNode::GridMapNode(): Node("grid_map_node") {
  initialGridMap();
  tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
  tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);
  audio_listener_ = this->create_subscription<audio_interfaces::msg::AudioMsg>("/audio_msg", 10, 
    std::bind(&GridMapNode::audio_listen_cb, this, std::placeholders::_1));
  pub_ = this->create_publisher<grid_map_msgs::msg::GridMap>("/grid_map", 10);
  timer_update_ = this->create_wall_timer(std::chrono::milliseconds(100), std::bind(&GridMapNode::updategrid_cb, this));
  timer_publish_ = this->create_wall_timer(std::chrono::milliseconds(1000), std::bind(&GridMapNode::publishGridMap_cb, this));
}

GridMapNode::~GridMapNode(){}

void GridMapNode::initialGridMap(){
  map_.setFrameId("map");
  map_.setGeometry(grid_map::Length(20, 20), 0.2);
  map_.add("energy");
}
void GridMapNode::updategrid_cb(){
  geometry_msgs::msg::TransformStamped tf;
  try {
    tf = tf_buffer_->lookupTransform("map", "base_link", tf2::TimePointZero);
  } catch (tf2::TransformException &ex) {
    RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000,
        "TF not ready: %s", ex.what());
    return;
  }
  double x = tf.transform.translation.x;
  double y = tf.transform.translation.y;
  audio_interfaces::msg::AudioMsg::SharedPtr audio_copy;
  {
    std::lock_guard<std::mutex> lock(audio_mutex_);
    audio_copy = latest_audio_;
  }
  if (!audio_copy) return;
  float energy = audio_copy->dbfs;
  grid_map::Position pos(x, y);
  if (map_.isInside(pos)) {
    map_.atPosition("energy", pos) = energy;
  }
}
void GridMapNode::publishGridMap_cb(){
  auto msg = grid_map::GridMapRosConverter::toMessage(map_);
  pub_->publish(std::move(msg));
}
void GridMapNode::audio_listen_cb(const audio_interfaces::msg::AudioMsg::SharedPtr msg){
  std::lock_guard<std::mutex> lock(audio_mutex_);
  latest_audio_ = msg;
}

int main(int argc, char **argv){
  rclcpp::init(argc, argv);
  auto node = std::make_shared<GridMapNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}