#include <rclcpp/rclcpp.hpp>
#include <tf2_ros/transform_broadcaster.h>
#include <geometry_msgs/msg/transform_stamped.hpp>

class tf_node: public rclcpp::Node {
public:
  tf_node() : Node("tf_node"), x_(0) {
    tf_broadcaster_ = std::make_shared<tf2_ros::TransformBroadcaster>(this);

    tf_timer_ = this->create_wall_timer(std::chrono::milliseconds(100),std::bind(&tf_node::tf_cb, this));
  }
  ~tf_node(){}
private:
  void tf_cb(){
    geometry_msgs::msg::TransformStamped t;

    t.header.stamp = this->get_clock()->now();
    t.header.frame_id = "map";
    t.child_frame_id = "base_link";
    x_ += 0.01;
    t.transform.translation.x = x_;
    t.transform.translation.y = 0.0;
    t.transform.translation.z = 0.0;
    t.transform.rotation.x = 0.0;
    t.transform.rotation.y = 0.0;
    t.transform.rotation.z = 0.0;
    t.transform.rotation.w = 1.0;

    tf_broadcaster_->sendTransform(t);
  }
private:
  std::shared_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;
  rclcpp::TimerBase::SharedPtr tf_timer_;
  double x_;

};
int main(int argc, char **argv){
  rclcpp::init(argc, argv);
  auto node = std::make_shared<tf_node>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}