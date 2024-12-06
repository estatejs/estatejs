//
// Created by scott on 4/5/20.
//
#pragma once

#include <estate/runtime/result.h>
#include "estate/internal/flatbuffers_util.h"
#include "estate/internal/logging.h"

#include <memory>

namespace estate {
    template<
            typename TConfig,
            typename TRequestProto,
            typename TResponseProto,
            typename TServiceProvider,
            typename TRequestContext,
            typename TBuffer,
            void Executor(const TConfig &config, std::shared_ptr<TServiceProvider>, TBuffer &&, std::shared_ptr<TRequestContext>)
    >
    struct Processor {
        //[sic] do not delete these. CLion incorrectly says they're unused.
        using Config = TConfig;
        using ServiceProvider = TServiceProvider;
        using RequestContext = TRequestContext;
        explicit Processor(const TConfig config, std::shared_ptr<TServiceProvider> service_provider) : _config(config), _service_provider(service_provider) {}
        void post(TBuffer &&request_buffer, std::shared_ptr<TRequestContext> request_context) {
            Executor(_config, _service_provider, std::move(request_buffer), std::move(request_context));
        }
    private:
        const TConfig _config;
        std::shared_ptr<TServiceProvider> _service_provider;
    };
}
