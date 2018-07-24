cmake_minimum_required(VERSION 3.2)
project(bo_app VERSION 1.0.1 LANGUAGES C)

add_executable(bo src/main.c)

target_compile_options(bo PRIVATE $<$<C_COMPILER_ID:GNU>:
    -Wall
    -Wextra
#    -Wduplicated-cond
#    -Wduplicated-branches
    -Wlogical-op
#    -Wrestrict
#    -Wnull-dereference
    -Wjump-misses-init
    -Wdouble-promotion
    -Wshadow
    -fstrict-aliasing
    -Wformat=2
>)

target_link_libraries(bo libbo)
