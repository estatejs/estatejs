//
// Created by scott on 5/6/20.
//

#pragma once

#include <estate/internal/innerspace/innerspace-client.h>
#include <estate/runtime/protocol/worker_process_interface_generated.h>

#include <estate/runtime/numeric_types.h>
#include <estate/runtime/code.h>
#include <cstddef>

#if defined DLL_EXPORTS
#if defined WIN32
#define LIB_API(RetType) extern "C" __declspec(dllexport) RetType
#else
#define LIB_API(RetType) extern "C" RetType __attribute__((visibility("default")))
#endif
#else
#if defined WIN32
#define LIB_API(RetType) extern "C" __declspec(dllimport) RetType
#else
#define LIB_API(RetType) extern "C" RetType
#endif
#endif

using namespace estate;

LIB_API(void) init(const char* config_file);

typedef void (*OnResponseCallback)(Code code, const u8 *response_bytes, const size_t response_size);

LIB_API(void) send_setup_worker_request(const char *log_context_str, const WorkerId worker_id, const u8 *buffer, size_t buffer_size, OnResponseCallback callback);
LIB_API(void) send_delete_worker_request(const char *log_context_str, const WorkerId worker_id, const u8 *buffer, size_t buffer_size, OnResponseCallback callback);
