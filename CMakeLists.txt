cmake_minimum_required(VERSION 3.15)
project(TRPO LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Находим Python 3 (современный и правильный способ)
find_package(Python3 3.14 EXACT COMPONENTS Interpreter Development REQUIRED)

# Подключаем pybind11
add_subdirectory(third_party/pybind11)

# Создаём целевой файл
add_executable(app main.cpp)

# ПРАВИЛЬНЫЙ способ указать, куда класть .exe (в папку bin/app)
set_target_properties(app PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/app"
)

# Линкуем с Python и pybind11
target_link_libraries(app PRIVATE Python3::Python pybind11::embed)

# Эта строка не обязательна, так как Python3::Python уже включает нужные пути,
# но оставить её не повредит
target_include_directories(app PRIVATE ${Python3_INCLUDE_DIRS})