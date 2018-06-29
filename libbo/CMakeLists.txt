cmake_minimum_required(VERSION 3.5)
project(libbo VERSION 1.0.0 LANGUAGES C)

#Make sure that custom modules are found
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)

find_package(BISON)
find_package(FLEX)


##############################################
# Create target and set properties

bison_target(BisonParser src/bo.y ${CMAKE_CURRENT_BINARY_DIR}/parser.c)
flex_target(FlexScanner src/bo.l ${CMAKE_CURRENT_BINARY_DIR}/tokens.c)
add_flex_bison_dependency(FlexScanner BisonParser)

add_library(libbo
    src/library.c
    ${BISON_BisonParser_OUTPUTS}
    ${FLEX_FlexScanner_OUTPUTS}
)

#Add an alias so that library can be used inside the build tree, e.g. when testing
add_library(BO::libbo ALIAS libbo)

#Set target properties
target_include_directories(libbo
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(libbo PRIVATE $<$<CXX_COMPILER_ID:GNU>:
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

##############################################
# Installation instructions

include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/BO)

configure_file(src/library_version.h.in library_version.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

install(TARGETS libbo
    EXPORT libbo-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

#This is required so that the exported target has the name BO and not bo
set_target_properties(libbo PROPERTIES EXPORT_NAME BO)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#Export the targets to a script
install(EXPORT libbo-targets
  FILE
    BOTargets.cmake
  NAMESPACE
    BO::
  DESTINATION
    ${INSTALL_CONFIGDIR}
)

#Create a ConfigVersion.cmake file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/BOConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/BOConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/BOConfig.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

#Install the config, configversion and custom find modules
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/BOConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/BOConfigVersion.cmake
    DESTINATION ${INSTALL_CONFIGDIR}
)

export(EXPORT libbo-targets FILE ${CMAKE_CURRENT_BINARY_DIR}/BOTargets.cmake NAMESPACE BO::)

#Register package in user's package registry
export(PACKAGE BO)

add_subdirectory(test)
