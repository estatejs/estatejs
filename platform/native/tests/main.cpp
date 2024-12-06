#include "gtest/gtest.h"
#include "logging.h"
#include <iostream>

int main(int argc, char **argv) {
#ifndef NDEBUG
    std::cout << "Running in DEBUG mode" << std::endl;
#else
    std::cout << "Running in RELEASE mode" << std::endl;
#endif
    init_default_logging_level();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}