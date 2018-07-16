enable_language(CXX)

# Download and unpack googletest at configure time
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/GoogleTest-CMakeLists.txt.in ${CMAKE_BINARY_DIR}/googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
                 ${CMAKE_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)

# Now simply link against gtest or gtest_main as needed. Eg
add_executable(libbo_test
                   src/main.cpp
                   src/test_helpers.cpp
                   src/output.cpp
                   src/errors.cpp
                   src/input.cpp
                   src/config.cpp
                   src/span.cpp
                   src/boolean.cpp
               )

target_compile_features(libbo_test PRIVATE cxx_auto_type)
target_link_libraries(libbo_test gtest_main libbo)

# Gain access to internal headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
