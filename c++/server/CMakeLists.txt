cmake_minimum_required(VERSION 3.0)
project(ndnperfserver)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -O2")

set(SOURCE_FILES
        concurrentqueue.h
        blockingconcurrentqueue.h
        server.cpp)

find_package(Boost COMPONENTS system filesystem thread REQUIRED)

find_library(ndn-cxx REQUIRED)
find_library(pthread REQUIRED)

add_executable(ndnperfserver ${SOURCE_FILES})

target_link_libraries(ndnperfserver ndn-cxx ${Boost_LIBRARIES} pthread)
