# CMakeList.txt : CMake project for ONI_capture, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("ONI_capture")

include_directories(
    "D:\\Program Files\\OpenNI2\\Include"
    "D:\\opencv\\opencv4.2.0\\build\\include"
)

link_directories(
    "D:\\Program Files\\OpenNI2\\Lib"
    "D:\\opencv\\opencv4.2.0\\build\\x64\\vc14\\lib"
)

# Add source to this project's executable.
add_executable (ONI_capture "ONI_capture.cpp")

target_link_libraries(ONI_capture 
    opencv_world420d
    OpenNI2
)
# TODO: Add tests and install targets if needed.
