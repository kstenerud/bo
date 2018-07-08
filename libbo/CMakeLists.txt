project(libo VERSION 1.0.0 LANGUAGES C)

find_package(BISON)
find_package(FLEX)

bison_target(BisonParser src/bo.y ${CMAKE_CURRENT_BINARY_DIR}/parser.c)
flex_target(FlexScanner src/bo.l ${CMAKE_CURRENT_BINARY_DIR}/tokens.c)
add_flex_bison_dependency(FlexScanner BisonParser)

add_library(libbo
    src/library.c
    src/parser.c
    ${BISON_BisonParser_OUTPUTS}
    ${FLEX_FlexScanner_OUTPUTS}
)

target_include_directories(libbo
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src
)

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
