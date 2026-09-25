cmake_minimum_required(VERSION 3.16)
project(main)

set(CMAKE_CXX_STANDARD 17)

# Жёстко указываем правильный Python
set(Python3_ROOT_DIR "C:/Users/Алексей/AppData/Local/Python/pythoncore-3.14-64")

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

add_executable(TRPO main.cpp)

target_include_directories(TRPO PRIVATE ${Python3_INCLUDE_DIRS})
target_link_libraries(TRPO PRIVATE Python3::Python)

# Копируем DLL из правильного места
if(WIN32)
    add_custom_command(TARGET TRPO POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${Python3_ROOT_DIR}/python314.dll"
            $<TARGET_FILE_DIR:TRPO>
    )
endif()