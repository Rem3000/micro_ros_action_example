cmake_minimum_required(VERSION 3.8)
project(bot_interfaces)

find_package(ament_cmake REQUIRED)
# find_package(rclcpp REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(action_msgs REQUIRED)

## Declare the action files
set(action_files
  "action/MoveDistance.action"
)

## Generate actions
rosidl_generate_interfaces(${PROJECT_NAME}
  ${action_files}
  DEPENDENCIES builtin_interfaces action_msgs
)

ament_export_dependencies(rosidl_default_runtime)

ament_package()
