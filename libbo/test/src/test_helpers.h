#pragma once

#include <gtest/gtest.h>
#include <bo/bo.h>

void assert_conversion(const char* input, const char* expected_output);

void assert_spanning_conversion(const char* input, int split_point, const char* expected_output);

void assert_failed_conversion(int buffer_length, const char* input);
