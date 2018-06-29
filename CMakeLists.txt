cmake_minimum_required(VERSION 3.2)
project(bo VERSION 1.0.0 LANGUAGES C)

include(GNUInstallDirs)

enable_testing()

add_subdirectory(libbo)
add_subdirectory(bo_app)
