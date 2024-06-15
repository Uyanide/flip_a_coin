cmake_minimum_required(VERSION 3.25)

project(coin)

set(CMAKE_CXX_STANDARD 17)

find_program(WINDRES_EXECUTABLE NAMES windres)

if(WINDRES_EXECUTABLE)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resources.o
        COMMAND ${WINDRES_EXECUTABLE} -I${CMAKE_CURRENT_SOURCE_DIR} -i${CMAKE_CURRENT_SOURCE_DIR}/rc_for_cmake.rc
        -o ${CMAKE_CURRENT_BINARY_DIR}/resources.o)
    set(COIN_RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/resources.o)
else()
    message(FATAL_ERROR "windres not found")
endif()

add_definitions(-DUNICODE -D_UNICODE)

add_executable(coin main.cpp gif_play.cpp ${COIN_RESOURCES})

target_compile_options(coin PRIVATE -mwindows)
target_link_options(coin PRIVATE -mwindows -municode)

target_link_libraries(coin gdiplus)