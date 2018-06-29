cmake_minimum_required(VERSION 3.2)
project(bo_app VERSION 1.0.0 LANGUAGES C)

add_executable(bo src/main.c)

target_link_libraries(bo libbo)
