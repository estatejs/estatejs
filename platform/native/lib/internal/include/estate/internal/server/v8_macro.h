//
// Originally written by Scott R. Jones.
// Copyright (c) 2021 Warpdrive Technologies, Inc. All rights reserved.
//

#pragma once

#include "./server_decl.h"

#define V8_SCOPE(ISOLATE) \
        auto isolate = ISOLATE; \
        v8::HandleScope handle_scope(isolate); \
        auto context = isolate->GetCurrentContext(); \
        v8::Context::Scope context_scope { context }
#define V8_ESCAPABLE_SCOPE(ISOLATE) \
        auto isolate = ISOLATE; \
        v8::EscapableHandleScope handle_scope(isolate); \
        auto context = isolate->GetCurrentContext(); \
        v8::Context::Scope context_scope { context }
#define V8_SCOPE_INHERIT_CONTEXT(ISOLATE, CONTEXT) \
        auto isolate = ISOLATE; \
        v8::HandleScope handle_scope(isolate); \
        auto context = v8::Local<v8::Context>::New(isolate, CONTEXT); \
        v8::Context::Scope context_scope { context }
#define _V8_CALL_TO_FAILED_PREFIX "[{}]: "
#define V8_THROW_FMT(FROM, ERROR_MESSAGE, ...) \
        const auto error_message = fmt::format(_V8_CALL_TO_FAILED_PREFIX ERROR_MESSAGE, FROM, __VA_ARGS__); \
        isolate->Enter();                                 \
        isolate->ThrowException(v8::Exception::Error(V8_STR(error_message))); \
        isolate->Exit()
#define V8_THROW(FROM, ERROR_MESSAGE) \
        const auto error_message = fmt::format(_V8_CALL_TO_FAILED_PREFIX "{}", FROM, ERROR_MESSAGE); \
        isolate->Enter();                                 \
        isolate->ThrowException(v8::Exception::Error(V8_STR(error_message))); \
        isolate->Exit()
#define V8_UNWRAP_OBJECT(OBJECT) (static_cast<data::Object*>(v8::Local<v8::External>::Cast((OBJECT)->GetInternalField(0))->Value()))
#define V8_STRING(STR, LEN) (v8::String::NewFromUtf8(isolate, STR, v8::NewStringType::kNormal, LEN).ToLocalChecked())
#define V8_STR(STR) V8_STRING(STR.data(), STR.length())
#define V8_RO_FUNC_PROP_ATTR static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontEnum)
#define V8_EXPORT_FUNCTION(m, ns, n) \
{\
    static const std::string name_str{ESTATE_STRINGIFY(n)};\
    const auto name = V8_STR(name_str);\
    auto func_template = v8::FunctionTemplate::New(isolate, ns :: n);\
    auto func = func_template->GetFunction(context).ToLocalChecked();\
    m->SetSyntheticModuleExport(isolate, name, func).Check();\
}
#define V8_EXPORT_CTOR(m, ns, n, ot) \
{ \
    v8::NamedPropertyHandlerConfiguration handler_config{native::object::on_##ot##_property_get,\
                                                         native::object::on_##ot##_property_set,\
                                                         nullptr,\
                                                         native::object::on_##ot##_property_delete,\
                                                         nullptr,\
                                                         nullptr,\
                                                         nullptr,\
                                                         v8::Local<v8::Value>(),\
                                                         v8::PropertyHandlerFlags::kNone};\
    {\
        auto func_template = v8::FunctionTemplate::New(isolate, ns :: n);\
        auto instance_template = func_template->InstanceTemplate();\
        instance_template->SetInternalFieldCount(1);\
        instance_template->SetHandler(handler_config);\
        static const std::string name_str{ESTATE_STRINGIFY(n)};\
        const auto name = V8_STR(name_str);\
        auto func = func_template->GetFunction(context).ToLocalChecked();\
        m->SetSyntheticModuleExport(isolate, name, func).Check();\
    }\
}

#define V8_DEFINE_OBJECT_FUNCTION_N(t, ns, n, s) \
{ \
static const std::string name_str{s}; \
const auto name = V8_STR(name_str); \
const auto func_templ = v8::FunctionTemplate::New(isolate, ns :: n); \
t->Set(name, func_templ, V8_RO_FUNC_PROP_ATTR); \
}

#define V8_DEFINE_OBJECT_FUNCTION(t, ns, n) \
{ \
    static const std::string name_str{ESTATE_STRINGIFY(n)}; \
    const auto name = V8_STR(name_str); \
    const auto func_templ = v8::FunctionTemplate::New(isolate, ns :: n); \
    t->Set(name, func_templ, V8_RO_FUNC_PROP_ATTR); \
}

#define V8_EXPORT_OBJECT(m, t, name_str) \
{ \
    static const std::string name_str_{name_str}; \
    const auto name = V8_STR(name_str_); \
    auto object = t->NewInstance(context).ToLocalChecked(); \
    m->SetSyntheticModuleExport(isolate, name, object).Check(); \
}

#define V8_COMPILE_MODULE(m, worker_name, code_str, file_name_str) \
v8::Local<v8::Module> m; \
{                                                                \
    const auto origin_name_str = fmt::format(ESTATE_MODULE_SOURCE_FILE_NAME_FORMAT, worker_name, file_name_str);\
    v8::ScriptCompiler::Source source{V8_STR(code_str),\
                                      v8::ScriptOrigin{V8_STR(origin_name_str),\
                                                       v8::Integer::New(isolate, 0),\
                                                       v8::Integer::New(isolate, 0),\
                                                       v8::False(isolate),\
                                                       v8::Local<v8::Integer>(),\
                                                       v8::Local<v8::Value>(),\
                                                       v8::False(isolate),\
                                                       v8::False(isolate),\
                                                       v8::True(isolate) /*is ES6 module*/}};\
    \
    v8::TryCatch try_catch{isolate};\
    if (!v8::ScriptCompiler::CompileModule(isolate, &source).ToLocal(&m)) {\
        if (try_catch.HasCaught()) {\
            auto ex = create_script_exception(log_context, isolate, try_catch);\
            log_script_exception(log_context, ex);\
            log_error(log_context, "Failed to compile module {} due to script exception", file_name_str);\
            return Result::Error(ex);\
        }\
        assert(false);\
        log_error(log_context, "Unable to compile module {} for an unknown reason", file_name_str);\
        return Result::Error(Code::ScriptEngine_FailedToCompileModuleUnknownReason);\
    }\
}
