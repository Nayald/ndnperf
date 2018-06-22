cmake_minimum_required(VERSION 3.9)
project(ndnperf)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

set(SOURCE_FILES main.cpp client.cpp client.h module.h)

find_package(Boost COMPONENTS system thread program_options REQUIRED)

find_library(ndn-cxx REQUIRED)
find_library(pthread REQUIRED)

add_executable(ndnperf ${SOURCE_FILES})

target_link_libraries(ndnperf ndn-cxx ${Boost_LIBRARIES} pthread)