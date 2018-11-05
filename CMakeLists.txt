cmake_minimum_required(VERSION 2.8)

project(calc_imaginary_ir)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")

add_executable(calc_imaginary_ir
	src/main.cpp
)
