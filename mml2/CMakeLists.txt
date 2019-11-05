cmake_minimum_required(VERSION 3.7)

add_compile_options(-std=c++17 -Wall -O2)

project(mml)

file(GLOB SRC "src/*.hpp" "src/*.cpp")

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
    SDL2
    sndfile
    )
