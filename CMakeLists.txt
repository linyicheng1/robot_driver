cmake_minimum_required(VERSION 3.20)
project(robot_driver)

set(CMAKE_CXX_STANDARD 14)

add_executable(test_serial_port serial_port.cpp test/test_serial_port.cpp)
target_link_libraries(test_serial_port -lboost_system -lpthread -lboost_thread)

add_executable(test_imu_alb serial_port.cpp test/test_imu_alb.cpp imu_alb.cpp)
target_link_libraries(test_imu_alb -lboost_system -lpthread -lboost_thread)

add_executable(test_can_yh can_yh.cpp test/test_can_yh.cpp)
