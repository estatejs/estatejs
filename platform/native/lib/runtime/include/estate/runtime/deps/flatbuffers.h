//
// Created by scott on 4/23/20.
//
#pragma once

#include <flatbuffers/base.h>

#define FLATBUFFERS_DEBUG_VERIFICATION_FAILURE 1
#define FLATBUFFERS_HAS_STRING_VIEW 1

#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/reflection.h>

#include <memory>

namespace fbs {
    template<typename T>
    using Offset = flatbuffers::Offset<T>;
    using Builder = flatbuffers::FlatBufferBuilder;
    using BuilderS = std::shared_ptr<Builder>;
    template<typename T>
    using Vector = flatbuffers::Vector<T>;
    using String = flatbuffers::String;
}
