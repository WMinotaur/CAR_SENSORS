cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(CAR-STM)


add_subdirectory(src)
add_subdirectory(inc)

