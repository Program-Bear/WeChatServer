cmake_minimum_required(VERSION 3.9)
project(WeChat)

set(CMAKE_CXX_STANDARD 11)

add_executable(WeChat main.cpp Client.h Protocal.h command_def.h ack_def.h)