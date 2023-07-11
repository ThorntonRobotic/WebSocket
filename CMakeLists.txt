#https://code.visualstudio.com/docs/cpp/CMake-linux
# apt-get install cmake
# TODO
# Is there a better way to handle include files, should we copy to build ?
# Build python bindings examples, see where they get installed

macro(global_config)
  set(CMAKE_VERBOSE_MAKEFILE ON)
  set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

  include(GNUInstallDirs)

  # Setup compiler switches
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++17 -fPIC")
  else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g -Wall -std=c++17 -fPIC")
  endif()


 # find_package(CUDA QUIET REQUIRED)
 # include_directories(${CUDA_INCLUDE_DIRS})
  #
  #link_directories(${CMAKE_SOURCE_DIR}/../WebRTC_R32.7.3)
  # MAC set(GLOBAL_LIBRARIES  ${CUDA_LIBRARIES}  dl pthread)
  if (WIN32)
    set(GLOBAL_LIBRARIES pthread ws2_32 mswsock)
  else()
    set(GLOBAL_LIBRARIES  pthread dl)
  endif (WIN32)
  

endmacro()



cmake_minimum_required(VERSION 3.10.0)
project(WebSocket LANGUAGES CXX C VERSION 0.1.0)

set(TARGET WebSocket)

global_config()

include_directories(${PROJECT_NAME}
  #../WebRTC_R32.7.3/webrtc_headers
  ./asio/asio/include
  ./websocketpp
)

add_executable(wsServerCallback wsServerCallback.cpp)
set_property(TARGET wsServerCallback PROPERTY CXX_STANDARD 17)
target_link_libraries(wsServerCallback ${GLOBAL_LIBRARIES})
set_target_properties(wsServerCallback PROPERTIES
RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/debug
RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/release)

add_executable(wsServerClass wsServerClass.cpp)
set_property(TARGET wsServerClass PROPERTY CXX_STANDARD 17)
target_link_libraries(wsServerClass ${GLOBAL_LIBRARIES})
set_target_properties(wsServerClass PROPERTIES
RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/debug
RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/release)

add_executable(wsClientClass wsClientClass.cpp)
set_property(TARGET wsClientClass PROPERTY CXX_STANDARD 17)
target_link_libraries(wsClientClass ${GLOBAL_LIBRARIES})
set_target_properties(wsClientClass PROPERTIES
RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/debug
RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/release)



