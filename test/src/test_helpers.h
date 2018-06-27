#pragma once

#include <gtest/gtest.h>
#include <bo/bo.h>

void assert_conversion(const char* input, const char* expected_output);

void assert_failed_conversion(const char* input, const char* expected_output);
