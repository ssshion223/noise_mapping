from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():

    return LaunchDescription([
        Node(
            package='noise_mapping',
            executable='audio_node',
            name='audio_node',
            output='screen'
        ),
        Node(
            package='noise_mapping',
            executable='noisemapping_node',
            name='noisemapping_node',
            output='screen'
        ),
        Node(
            package='noise_mapping',
            executable='tf_node',
            name='tf_node',
            output='screen'
        )
    ])