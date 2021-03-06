project(libbo VERSION 1.0.1 LANGUAGES C)

add_library(libbo
    src/library.c
    src/parser.c
)

target_include_directories(libbo
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src
)

target_compile_options(libbo PRIVATE $<$<C_COMPILER_ID:GNU>:
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

configure_file(src/library_version.h.in library_version.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

include(GNUInstallDirs)

# 'make install' to correct locations (GNUInstallDirs)
install(TARGETS libbo EXPORT LibBoConfig
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Make project importable from install directory
install(EXPORT LibBoConfig DESTINATION share/LibBo/cmake)

# Make project importable from build directory
export(TARGETS libbo FILE LibBoConfig.cmake)

add_subdirectory(test)
