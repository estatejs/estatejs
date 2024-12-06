//
// Created by scott on 4/5/20.
//
#pragma once

#include <boost/config/helper_macros.hpp>

#include "./version_generated.h"

#define ESTATE_VERSION \
BOOST_STRINGIZE(ESTATE_MAJOR_VERSION) "." \
BOOST_STRINGIZE(ESTATE_MINOR_VERSION) "." \
BOOST_STRINGIZE(ESTATE_PATCH_VERSION) "-" \
ESTATE_BUILD_VERSION
