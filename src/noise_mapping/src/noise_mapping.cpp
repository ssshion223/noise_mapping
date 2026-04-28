#include "noise_mapping.hpp"


GridMapNode::GridMapNode(): Node("grid_map_node") {
  initialGridMap();
  compute_kernel(R_, Gaussian_kernel_, sigma_);
  tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
  tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);
  audio_listener_ = this->create_subscription<audio_interfaces::msg::AudioMsg>("/audio_msg", 10, 
    std::bind(&GridMapNode::audio_listen_cb, this, std::placeholders::_1));
  local_pub_ = this->create_publisher<grid_map_msgs::msg::GridMap>("/local_grid_map", 10);
  global_pub_ = this->create_publisher<grid_map_msgs::msg::GridMap>("/global_grid_map", 10);
  local_update_timer_ = this->create_wall_timer(std::chrono::milliseconds(200), 
  std::bind(&GridMapNode::local_update_cb, this));
  global_update_timer_ = this->create_wall_timer(std::chrono::milliseconds(3000), 
  std::bind(&GridMapNode::global_update_cb, this));
}

GridMapNode::~GridMapNode(){}

void GridMapNode::initialGridMap(){
  global_map_.setFrameId("map");
  global_map_.setGeometry(grid_map::Length(global_height_, global_weight_), resolution_);
  global_map_.add("energy");
  local_map_.setFrameId("base_link");
  // local_map_.setGeometry(grid_map::Length(local_height, local_weight, resolution_));
  local_map_.setGeometry(grid_map::Length((2*R_+1)*resolution_, (2*R_+1)*resolution_), resolution_);
  local_map_.add("energy");
}
void GridMapNode::compute_kernel(const int &R, std::vector<std::vector<float>> &k, const float &sigma){
  int size = 2 * R + 1; 
  k.resize(size, std::vector<float>(size));
  for (int i = -R; i <= R; i++) {
    for (int j = -R; j <=R; j++) {
      double x = i*resolution_;
      double y = j*resolution_;
      double r2 = x*x + y*y;
      k[i + R][j + R] = std::exp(-r2 / (2 * sigma * sigma));
    }
  }
}
void GridMapNode::local_update_cb(){
  audio_interfaces::msg::AudioMsg::SharedPtr audio_copy;
  {
    std::lock_guard<std::mutex> lock(audio_mutex_);
    audio_copy = latest_audio_;
  }
  if (!audio_copy) return;
  float energy = audio_copy->dbfs;
  grid_map::Index idx0(0, 0);
  local_map_.at("energy", idx0) = energy;
  for (int i = 0; i <= 2*R_; i++) {
    for (int j = 0; j <= 2*R_; j++) {
      grid_map::Index idx(i, j);
      if (!local_map_.isValid(idx)) continue;
      local_map_.at("energy", idx) = energy * Gaussian_kernel_[i][j];
    }
  }
  auto msg = grid_map::GridMapRosConverter::toMessage(local_map_);
  local_pub_->publish(std::move(msg));
  // local_map_.clear("energy");
}
void GridMapNode::global_update_cb(){
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