cmake_minimum_required(VERSION 3.14)
project(forge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# llama.cpp
#
# Point the build at a llama.cpp checkout with -DLLAMA_CPP_DIR=/path/to/llama.cpp
# or the LLAMA_CPP_DIR environment variable. If neither is set, a sibling
# directory next to this project is used.
# ---------------------------------------------------------------------------
if(NOT LLAMA_CPP_DIR AND DEFINED ENV{LLAMA_CPP_DIR})
    set(LLAMA_CPP_DIR "$ENV{LLAMA_CPP_DIR}")
endif()
set(LLAMA_CPP_DIR "${LLAMA_CPP_DIR}" CACHE PATH "Path to a llama.cpp checkout")

if(NOT LLAMA_CPP_DIR AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../llama.cpp")
    get_filename_component(LLAMA_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../llama.cpp" ABSOLUTE)
    message(STATUS "LLAMA_CPP_DIR not set; using sibling checkout: ${LLAMA_CPP_DIR}")
endif()

# Locate the headers and shared library directly. This works for an in-tree
# llama.cpp build (build/bin), an installed prefix, or a system package. The
# CMake config llama.cpp exports is deliberately not used: in a build tree it
# resolves its include dir to the install prefix, which does not exist yet.
find_path(LLAMA_INCLUDE_DIR llama.h
    HINTS "${LLAMA_CPP_DIR}" PATH_SUFFIXES include)
find_path(GGML_INCLUDE_DIR ggml.h
    HINTS "${LLAMA_CPP_DIR}" PATH_SUFFIXES ggml/include include)
find_library(LLAMA_LIBRARY NAMES llama
    HINTS "${LLAMA_CPP_DIR}" PATH_SUFFIXES build/bin build/src build/lib bin lib)

if(NOT LLAMA_INCLUDE_DIR OR NOT LLAMA_LIBRARY)
    message(FATAL_ERROR
        "Could not locate llama.cpp (headers: ${LLAMA_INCLUDE_DIR}, "
        "library: ${LLAMA_LIBRARY}).\n"
        "Build llama.cpp as a shared library, then configure with:\n"
        "  cmake -B build -DLLAMA_CPP_DIR=/path/to/llama.cpp")
endif()

add_library(forge_llama INTERFACE)
target_include_directories(forge_llama INTERFACE "${LLAMA_INCLUDE_DIR}")
if(GGML_INCLUDE_DIR)
    target_include_directories(forge_llama INTERFACE "${GGML_INCLUDE_DIR}")
endif()
target_link_libraries(forge_llama INTERFACE "${LLAMA_LIBRARY}")
set(FORGE_LLAMA_TARGET forge_llama)
message(STATUS "Using llama.cpp library: ${LLAMA_LIBRARY}")

# ---------------------------------------------------------------------------
# nlohmann/json
# ---------------------------------------------------------------------------
find_package(nlohmann_json CONFIG QUIET)
if(nlohmann_json_FOUND)
    set(FORGE_JSON_TARGET nlohmann_json::nlohmann_json)
else()
    find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp)
    if(NOT NLOHMANN_JSON_INCLUDE_DIR)
        message(FATAL_ERROR
            "Could not find nlohmann/json.hpp.\n"
            "Install it (Arch: pacman -S nlohmann-json, Debian/Ubuntu: apt install "
            "nlohmann-json3-dev) or pass -DNLOHMANN_JSON_INCLUDE_DIR=/path/to/include")
    endif()
    add_library(forge_json INTERFACE)
    target_include_directories(forge_json INTERFACE "${NLOHMANN_JSON_INCLUDE_DIR}")
    set(FORGE_JSON_TARGET forge_json)
    message(STATUS "Using nlohmann/json headers: ${NLOHMANN_JSON_INCLUDE_DIR}")
endif()

# ---------------------------------------------------------------------------
# forge
# ---------------------------------------------------------------------------
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(forge main.cpp)

target_include_directories(forge PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(forge PRIVATE
    ${FORGE_LLAMA_TARGET}
    ${FORGE_JSON_TARGET}
    Threads::Threads
)

# Optional: bake the model path in at configure time instead of editing main.cpp.
if(FORGE_MODEL_PATH)
    target_compile_definitions(forge PRIVATE MODEL_PATH="${FORGE_MODEL_PATH}")
    message(STATUS "Model path: ${FORGE_MODEL_PATH}")
endif()
