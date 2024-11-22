cmake_minimum_required(VERSION 3.29)
project(cub_os_hw C)

set(CMAKE_C_STANDARD 11)

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

target_compile_definitions(gwgd-libevent
        PRIVATE levent lpthread levent_pthreads
)
