cmake_minimum_required(VERSION 3.10)

# Project name
project(Box)


# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set global C++ flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pipe -O2 -march=native -g")

add_executable(player 
  src/main.cc
  src/Position.cc
  src/PositionData.cc
)
