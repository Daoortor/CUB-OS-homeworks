cmake_minimum_required(VERSION 3.29)
project(energy-drinks C)

set(CMAKE_C_STANDARD 11)

add_executable(energy-drinks
    energy-drinks.c
    lib.c
)
