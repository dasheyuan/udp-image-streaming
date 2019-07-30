cmake_minimum_required(VERSION 2.8)
project( lan_vid_pseudostream )

set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS} -pthread")

SET(OpenCV_DIR /home/chenyuan/3rdparty/opencv-3.2.0/build)
find_package( OpenCV REQUIRED )
add_executable( server Server.cpp PracticalSocket.cpp )
target_link_libraries( server ${OpenCV_LIBS} )
add_executable( client Client.cpp PracticalSocket.cpp )
target_link_libraries( client ${OpenCV_LIBS} )

add_executable( server-r Server-r.cpp PracticalSocket.cpp )
target_link_libraries( server-r ${OpenCV_LIBS} )
add_executable( client-r Client-r.cpp PracticalSocket.cpp )
target_link_libraries( client-r ${OpenCV_LIBS} )


link_directories("/home/chenyuan/3rdparty/openpose/build/src/openpose")
add_executable( server-d Server-D.cpp PracticalSocket.cpp ServDelegate.cpp)
target_link_libraries( server-d ${OpenCV_LIBS} openpose gflags)