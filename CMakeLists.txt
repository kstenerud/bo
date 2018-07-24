cmake_minimum_required(VERSION 3.2)
project(bo LANGUAGES C)

include(GNUInstallDirs)

enable_testing()

add_subdirectory(libbo)
add_subdirectory(bo_app)
