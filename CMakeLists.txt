cmake_minimum_required(VERSION 3.20)
project(StreamForge CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(AWSSDK REQUIRED COMPONENTS s3 dynamodb sns)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

include_directories(src)

# Main server
add_executable(streamforge src/main.cpp)
target_link_libraries(streamforge
    ${AWSSDK_LINK_LIBRARIES}
    OpenSSL::SSL OpenSSL::Crypto Threads::Threads)

# AWS integration test
add_executable(test_aws tests/test_aws.cpp)
target_link_libraries(test_aws
    ${AWSSDK_LINK_LIBRARIES}
    OpenSSL::SSL OpenSSL::Crypto Threads::Threads)

# Existing tests
add_executable(test_rb tests/test_ringbuffer.cpp)
target_link_libraries(test_rb Threads::Threads)

add_executable(test_pipeline tests/test_pipeline.cpp)
target_link_libraries(test_pipeline Threads::Threads)

add_executable(test_anomaly tests/test_anomaly.cpp)
target_link_libraries(test_anomaly Threads::Threads)
