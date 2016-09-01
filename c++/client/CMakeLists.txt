cmake_minimum_required(VERSION 3.0)
project(ndnperf)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2")

set(SOURCE_FILES
        client.cpp)

find_package(Boost COMPONENTS system REQUIRED)

find_library(ndn-cxx REQUIRED)
find_library(pthread REQUIRED)

add_executable(ndnperf ${SOURCE_FILES})

target_link_libraries(ndnperf ndn-cxx ${Boost_LIBRARIES} pthread)