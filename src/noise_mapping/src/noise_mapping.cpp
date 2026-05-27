#include "noise_mapping.hpp"

#include <algorithm>
#include <cmath>

GridMapNode::GridMapNode(): Node("grid_map_node") {
  initialGridMap();
  compute_kernel(R_, Gaussian_kernel_, sigma_);
  tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
  tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);
  audio_listener_ = this->create_subscription<audio_interfaces::msg::AudioMsg>("/audio_msg", 10, 
    std::bind(&GridMapNode::audio_listen_cb, this, std::placeholders::_1));
  local_pub_ = this->create_publisher<grid_map_msgs::msg::GridMap>("/local_grid_map", 10);
  global_pub_ = this->create_publisher<grid_map_msgs::msg::GridMap>("/global_grid_map", 10);
  local_update_timer_ = this->create_wall_timer(std::chrono::milliseconds(100), //do update and publish
  std::bind(&GridMapNode::local_update_cb, this));
  global_update_timer_ = this->create_wall_timer(std::chrono::milliseconds(100), //only do update
  std::bind(&GridMapNode::global_update_cb, this));
  global_pub_timer_ = this->create_wall_timer(std::chrono::milliseconds(3000), //only do update
  std::bind(&GridMapNode::global_pub_cb, this));
}

GridMapNode::~GridMapNode(){}

void GridMapNode::initialGridMap(){
  global_map_.setFrameId("map");
  global_map_.setGeometry(grid_map::Length(global_height_, global_weight_), resolution_);
  global_map_.add("energy");
  global_map_.add("max_energy");
  global_map_.add("sample_count");
  global_map_["energy"].setZero();
  global_map_["max_energy"].setZero();
  global_map_["sample_count"].setZero();
  local_map_.setFrameId("base_link");
  // local_map_.setGeometry(grid_map::Length(local_height, local_weight, resolution_));
  local_map_.setGeometry(grid_map::Length((2*R_+1)*resolution_, (2*R_+1)*resolution_), resolution_);
  local_map_.add("energy");
  local_map_.add("weight");
  local_map_["energy"].setZero();
  local_map_["weight"].setZero();
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
  std::lock_guard<std::mutex> lock(local_mutex_);
  audio_interfaces::msg::AudioMsg::SharedPtr audio_copy;
  {
    std::lock_guard<std::mutex> lock(audio_mutex_);
    audio_copy = latest_audio_;
  }
  if (!audio_copy) return;
  try {
    {
      std::lock_guard<std::mutex> lock(tf_mutex_);
      tf_ = tf_buffer_->lookupTransform("map", "base_link", tf2::TimePointZero);
      has_tf_ = true;
    }
  } catch (tf2::TransformException &ex) {
    RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000,
        "TF not ready: %s", ex.what());
    return;
  }

  float energy = audio_copy->norm_dbfs;
  for (int i = 0; i <= 2*R_; i++) {
    for (int j = 0; j <= 2*R_; j++) {
      grid_map::Index idx(i, j);
      local_map_.at("energy", idx) = energy * Gaussian_kernel_[i][j];
      local_map_.at("weight", idx) = Gaussian_kernel_[i][j];
    }
  }
  local_map_.setTimestamp(this->now().nanoseconds());
  auto msg = grid_map::GridMapRosConverter::toMessage(local_map_);
  local_pub_->publish(std::move(msg));
}
void GridMapNode::global_update_cb(){
  std::lock_guard<std::mutex> locklocal(local_mutex_);
  std::lock_guard<std::mutex> lockglobal(global_mutex_);
  geometry_msgs::msg::TransformStamped tf_copy;
  {
    std::lock_guard<std::mutex> lock(tf_mutex_);
    if (!has_tf_) {
      return;
    }
    tf_copy = tf_;
  }
  for (grid_map::GridMapIterator it(local_map_); !it.isPastEnd(); ++it) {
    grid_map::Position local_pos;
    local_map_.getPosition(*it, local_pos);
    double x = local_pos.x();
    double y = local_pos.y();
    double tx = tf_copy.transform.translation.x;
    double ty = tf_copy.transform.translation.y;
    double yaw = tf2::getYaw(tf_copy.transform.rotation);
    double xg = x * std::cos(yaw) - y * std::sin(yaw) + tx;
    double yg = x * std::sin(yaw) + y * std::cos(yaw) + ty;
    grid_map::Index idx_global;
    if (!global_map_.getIndex(grid_map::Position(xg, yg), idx_global)) {
      continue;
    }

    float &g = global_map_.at("energy", idx_global);
    float l = local_map_.at("energy", *it);
    float w = local_map_.at("weight", *it);
    if (!std::isfinite(l) || !std::isfinite(w) || w <= 1e-3f) {
      continue;
    }

    float &max_g = global_map_.at("max_energy", idx_global);
    float &count = global_map_.at("sample_count", idx_global);
    g = (g * count + l * w) / (count + w);
    max_g = std::max(max_g, l);
    count += w;
  }
}
void GridMapNode::global_pub_cb(){
  std::lock_guard<std::mutex> lock(global_mutex_);
  global_map_.setTimestamp(this->now().nanoseconds());
  auto msg = grid_map::GridMapRosConverter::toMessage(global_map_);
  global_pub_->publish(std::move(msg));
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
