cmake_minimum_required(VERSION 3.14)
project(forge)

set(CMAKE_CXX_STANDARD 17)

add_executable(forge main.cpp)

target_include_directories(forge PRIVATE
    ${CMAKE_SOURCE_DIR}
    /home/imaayush/code/llama.cpp
    /home/imaayush/code/llama.cpp/include
    /home/imaayush/code/llama.cpp/ggml/include
)

target_link_libraries(forge PRIVATE
    /home/imaayush/code/llama.cpp/build/bin/libllama.so
    pthread
)