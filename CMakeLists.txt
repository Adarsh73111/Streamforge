cmake_minimum_required(VERSION 3.20)
project(StreamForge VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED)

include_directories(
    /usr/local/include
    src
)

add_executable(streamforge src/main.cpp)
target_link_libraries(streamforge
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
    ${Boost_LIBRARIES}
)
target_compile_options(streamforge PRIVATE -O2 -Wall)

add_executable(test_rb tests/test_ringbuffer.cpp)
target_link_libraries(test_rb Threads::Threads)

add_executable(test_pipeline tests/test_pipeline.cpp)
target_link_libraries(test_pipeline Threads::Threads)

enable_testing()
add_test(NAME RingBufferTest  COMMAND test_rb)
add_test(NAME PipelineTest    COMMAND test_pipeline)
