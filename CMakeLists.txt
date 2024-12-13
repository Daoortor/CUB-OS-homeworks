cmake_minimum_required(VERSION 3.29)
project(cub_os_hw C)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVENT REQUIRED libevent)

add_executable(env
        env/env.c
)

add_executable(math-multithreading
        math-multithreading/dpnum.c
)

add_executable(prisoners
        prisoners/prisoners.c
)

add_executable(energy-drinks
        energy-drinks/energy-drinks/lib.c
        energy-drinks/energy-drinks/energy-drinks.c
)

add_executable(gwgd
        gwgd/app/basic/main.c
        gwgd/util/chlng.c
        gwgd/util/player.c
        gwgd/lib/lib.c
)

add_executable(gwgd-libevent
        gwgd/app/libevent/main.c
        gwgd/util/chlng.c
        gwgd/util/player.c
        gwgd/lib/lib.c
)

target_include_directories(gwgd-libevent PRIVATE ${LIBEVENT_INCLUDE_DIRS})
target_link_libraries(gwgd-libevent PRIVATE ${LIBEVENT_LIBRARIES})
