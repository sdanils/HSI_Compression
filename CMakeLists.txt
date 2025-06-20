cmake_minimum_required(VERSION 3.10)
project(HSI_compression CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")

if(NOT SOURCES)
    message(FATAL_ERROR "Не найдены исходные файлы (*.c) в папке src")
endif()
if(NOT HEADERS)
    message(WARNING "Не найдены заголовочные файлы (*.h) в папке include")
endif()

add_executable(hsi_compressor ${SOURCES} ${HEADERS})

target_include_directories(hsi_compressor PRIVATE include)

if(MSVC)
    target_compile_options(hsi_compressor PRIVATE /W4 /WX)
else()
    target_compile_options(hsi_compressor PRIVATE -Wall -Wextra -Werror -pedantic)
endif()