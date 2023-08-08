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

  # Ugly but ok for now
  link_directories(${CMAKE_SOURCE_DIR}/build/jsoncpp/src/lib_json)

 # find_package(CUDA QUIET REQUIRED)
 # include_directories(${CUDA_INCLUDE_DIRS})
  #
  #link_directories(${CMAKE_SOURCE_DIR}/../WebRTC_R32.7.3)
  # MAC set(GLOBAL_LIBRARIES  ${CUDA_LIBRARIES}  dl pthread)
  if (WIN32)
    set(GLOBAL_LIBRARIES pthread ws2_32 mswsock jsoncpp)
  else()
    set(GLOBAL_LIBRARIES  pthread dl jsoncpp)
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
  ./jsoncpp/include
)

set(JSONCPP_WITH_TESTS  OFF)
set(BUILD_SHARED_LIBS OFF)
set(BUILD_STATIC_LIBS ON)
set(BUILD_OBJECT_LIBS OFF)

add_subdirectory(jsoncpp)

foreach(s
    wsServerCallback
    wsServerClass
    wsClientClass
    wsJsonClient
    wsJsonServer
    wsEndpointTest
    )
  add_executable(${s} ${s}.cpp)
  set_property(TARGET ${s} PROPERTY CXX_STANDARD 17)
  target_link_libraries(${s} ${GLOBAL_LIBRARIES})
  set_target_properties(${s} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/debug
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/release)
endforeach()



