//
// Originally written by Scott R. Jones.
// Copyright (c) 2020 Warpdrive Technologies, Inc. All rights reserved.
//

#include "./logging.h"

void init_default_logging_level() {
    estate::LoggingConfig config{
            "TESTS",
            spdlog::level::trace
    };
    estate::init_logging(config, true);
}

void init_performance_logging_level() {
    estate::LoggingConfig config{
            "TESTS",
            spdlog::level::info
    };
    estate::init_logging(config, true);
}