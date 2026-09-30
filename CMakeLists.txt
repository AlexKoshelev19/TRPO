cmake_minimum_required(VERSION 3.20)
project(main)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

# Указываем путь к локальному рантайму, чтобы CMake искал Python именно там
set(Python_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/python_runtime/python-3.14.7-embed-amd64")

find_package(Python REQUIRED COMPONENTS Interpreter Development.Embed)

# Подключение pybind11
add_subdirectory(third_party/pybind11)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE pybind11::embed)
