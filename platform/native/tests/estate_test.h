//
// Created by scott on 4/1/20.
//

#pragma once

#include <estate/internal/serenity/handler/user.h>
#include <estate/internal/serenity/handler/admin.h>

#include <estate/internal/stopwatch.h>
#include <estate/internal/flatbuffers_util.h>
#include <estate/internal/innerspace/innerspace.h>
#include <estate/runtime/event.h>

#include <estate/internal/deps/boost.h>
#include <estate/runtime/model_types.h>
#include <estate/runtime/limits.h>

#include <condition_variable>
#include <fmt/format.h>
#include <iostream>

#define __BEGIN_TEST_SECTION(name) \
    __test_section = #name; \
    __test_section += " ";         \
    context.log_context = std::make_shared<LogContext>(make_test_log_context); \
    log_trace(*context.log_context, ">>>>>>>>>>> Begin: {}", estate::test::pad_right_string(__test_section, 78, '>'));
#define SUBTEST_BEGIN(name) __BEGIN_TEST_SECTION(name)
#define __END_TEST_SECTION \
    log_trace(*context.log_context, "<<<<<<<<<<< End: {}", estate::test::pad_right_string(__test_section, 80, '<'));\
    __test_section = ""; \
    context.log_context = nullptr;
#define SUBTEST_END __END_TEST_SECTION

#define ESTATE_ASSERT(v) ::estate::test::__ASSERT_EQ(#v, "true", (bool)v, true, __FILE__, __LINE__)
#define ESTATE_ASSERT_EQ(l, r) ::estate::test::__ASSERT_EQ(#l, #r, l, r, __FILE__, __LINE__)
#define ESTATE_ASSERT_LE(l, r) ::estate::test::__ASSERT_LE(#l, #r, l, r, __FILE__, __LINE__)
#define ESTATE_ASSERT_GE(l, r) ::estate::test::__ASSERT_GE(#l, #r, l, r, __FILE__, __LINE__)
#define test_data_dir ::estate::test::get_test_data_directory(::testing::UnitTest::GetInstance()->current_test_info()->test_suite_name(), ::testing::UnitTest::GetInstance()->current_test_info()->name())
#define make_test_log_context LogContext {  ::estate::test::gen_random_string(ESTATE_LOG_CONTEXT_LENGTH) }

#define SETUP(worker_id, user, setup_worker, delete_worker) \
std::string __test_section{};                \
test::Context context {}; \
__BEGIN_TEST_SECTION(Setup) \
context.services = std::move(test::setup_serenity_processors(*context.log_context, worker_id, user,setup_worker,delete_worker).unwrap()); \
context.package = std::move(test::util::setup_worker_from_directory(*context.log_context, context.services, test_data_dir, 0)); \
__END_TEST_SECTION

#define validate_object_property(p, _name, _value) \
{\
    ASSERT_EQ(p->name()->str(), _name); \
    ASSERT_EQ(p->value()->value_type(), ValueUnionProto_ObjectValueProto); \
    ASSERT_EQ(p->value()->value_as_ObjectValueProto()->json()->str(), _value); \
}

#define validate_string_property(p, _name, _value) \
{\
    ASSERT_EQ(p->name()->str(), _name); \
    ASSERT_EQ(p->value_bytes_nested_root()->value_type(), ValueUnionProto_StringValueProto); \
    ASSERT_EQ(p->value_bytes_nested_root()->value_as_StringValueProto()->value()->str(), _value); \
}

#define validate_boolean_property(p, _name, _value) \
{\
    ASSERT_EQ(p->name()->str(), _name); \
    ASSERT_EQ(p->value_bytes_nested_root()->value_type(), ValueUnionProto_BooleanValueProto); \
    ASSERT_EQ(p->value_bytes_nested_root()->value_as_BooleanValueProto()->value(), _value); \
}

#define validate_number_property(p, _name, _value) \
{\
    ASSERT_EQ(p->name()->str(), _name); \
    ASSERT_EQ(p->value_bytes_nested_root()->value_type(), ValueUnionProto_NumberValueProto); \
    ASSERT_EQ(p->value_bytes_nested_root()->value_as_NumberValueProto()->value(), _value); \
}

#define validate_data_reference_property(p, _name, _primary_key_str, _class_id) \
{ \
    ASSERT_EQ(p->name()->str(), _name); \
    ASSERT_EQ(p->value_bytes_nested_root()->value_type(), ValueUnionProto_DataReferenceValueProto); \
    auto h = p->value_bytes_nested_root()->value_as_DataReferenceValueProto(); \
    ASSERT_EQ(h->primary_key()->str(), _primary_key_str); \
    ASSERT_EQ(h->class_id(), _class_id);\
}

#define validate_null_property(p, _name) \
{\
ASSERT_EQ(p->name()->str(), _name);\
ASSERT_EQ(p->value_bytes_nested_root()->value_type(), ValueUnionProto::ValueUnionProto_NullValueProto);\
ASSERT_EQ(p->value_bytes_nested_root()->value_as_NullValueProto(), nullptr);\
}

#define validate_undef_property(p, _name) \
{\
ASSERT_EQ(p->name()->str(), _name);\
ASSERT_EQ(p->value_bytes_nested_root()->value_type(), ValueUnionProto::ValueUnionProto_UndefinedValueProto);\
ASSERT_FALSE(p->value_bytes_nested_root()->value());\
}

namespace estate::test {
    std::string pad_right_string(const std::string &str, const size_t num, const char paddingChar = ' ');

    template<typename L, typename R>
    void throw_assert_fail(const char *eq, const char *neq, const char *ls, const char *rs, L l, R r, const char *file, int line) {
        std::cout << "--- ASSERTION FAILED in " << file << " @ " << std::to_string(line) << std::endl;
        std::cout << fmt::format("--- For {} {} {}\n--- \"{}\" {} \"{}\"", ls, eq, rs, l, neq, r) << std::endl;
        throw std::domain_error("assertion failure");
    }

    template<typename L, typename R>
    void __ASSERT_GE(const char *ls, const char *rs, L l, R r, const char *file, int line) {
        if (l < r) {
            throw_assert_fail(">=", "<", ls, rs, l, r, file, line);
        }
    }

    template<typename L, typename R>
    void __ASSERT_LE(const char *ls, const char *rs, L l, R r, const char *file, int line) {
        if (l > r) {
            throw_assert_fail("<=", ">", ls, rs, l, r, file, line);
        }
    }

    template<typename L, typename R>
    void __ASSERT_EQ(const char *ls, const char *rs, L l, R r, const char *file, int line) {
        if (l != r) {
            throw_assert_fail("==", "!=", ls, rs, l, r, file, line);
        }
    }

    std::string gen_random_string(const int len);

    std::string get_test_data_directory(const std::string &test_suite, const std::string &test_name);

    struct TestPackage {
        WorkerId worker_id;
        WorkerVersion worker_version;
        Buffer<WorkerIndexProto> worker_index;
        std::vector<std::string> code;
        TestPackage(WorkerId worker_id, WorkerVersion worker_version, Buffer<WorkerIndexProto> worker_index, std::vector<std::string> code);
    };

    using TestPackageU = std::unique_ptr<TestPackage>;

    TestPackageU load_package(BufferPoolS buffer_pool, const std::string &package_dir);
    const BufferView<SetupWorkerRequestProto>
    create_setup_worker_request(const LogContext &log_context, fbs::Builder &builder, WorkerId worker_id, WorkerVersion worker_version, WorkerVersion previous_worker_version,
                              Buffer<WorkerIndexProto> &&worker_index, const std::vector<std::string> &code);
    const BufferView<DeleteWorkerRequestProto> create_delete_worker_request(const LogContext &log_context, fbs::Builder &builder, WorkerId worker_id,
                                                                        WorkerVersion worker_version);
    const BufferView<UserRequestProto> create_get_object_request(const LogContext &log_context, fbs::Builder &builder, WorkerId worker_id, WorkerVersion worker_version, ClassId class_id,
                                                                 const PrimaryKey &primary_key);
    const BufferView<UserRequestProto> create_call_service_method_request(const LogContext &log_context, fbs::Builder &builder, WorkerId worker_id, WorkerVersion worker_version,
                                                                          ClassId class_id, const PrimaryKey &primary_key, MethodId method_id,
                                                                          std::optional<std::vector<fbs::Offset<ValueProto>>> arguments,
                                                                          std::optional<std::vector<fbs::Offset<InboundDataDeltaProto>>> referenced_data_detlas);
    const BufferView<UserRequestProto> create_save_data_request(const LogContext &log_context, fbs::Builder &builder, WorkerId worker_id, WorkerVersion worker_version,
                                                                        std::vector<fbs::Offset<InboundDataDeltaProto>> data);
    template<typename TResponseProto>
    struct TestRequestContext;

    template<typename TResponseProto>
    struct TestResult {
        explicit TestResult(const BufferView<TResponseProto> &result) : result(result) {}
        const BufferView<TResponseProto> result;
    };

    template<typename TResponseProto>
    struct RequestContextWrapper {
        const BufferView<TResponseProto> &get_result() {
            ESTATE_ASSERT(result);
            return result->result;
        }
        std::shared_ptr<TestRequestContext<TResponseProto>> create_request_context() {
            return std::make_shared<TestRequestContext<TResponseProto>>(TestRequestContext<TResponseProto>{*this});
        }
        void wait(const LogContext &log_context) {
            done_event.wait_one();
        }
        void done(const LogContext &&log_context, const BufferView<TResponseProto> &r) {
            result = std::make_unique<TestResult<TResponseProto>>(r);
            done_event.notify_one();
        }
    private:
        Event done_event{};
        std::unique_ptr<TestResult<TResponseProto>> result{};
    };

    template<typename TResponseProto>
    struct TestRequestContext {
        TestRequestContext(RequestContextWrapper<TResponseProto> &request_context_wrapper) : request_context_wrapper(request_context_wrapper) {
        }
        void async_respond(const LogContext &log_context, const BufferView<TResponseProto> &r, std::optional<std::function<void()>> and_then) {
            request_context_wrapper.done(std::move(log_context), r);
        }
    private:
        RequestContextWrapper<TResponseProto> &request_context_wrapper;
    };

    using TestUserProcessor = Processor<
            UserProcessorConfig,
            UserRequestProto,
            WorkerProcessUserResponseProto,
            UserServiceProvider,
            TestRequestContext<WorkerProcessUserResponseProto>,
            BufferView<UserRequestProto>,
            execute>;
    using TestUserProcessorS = std::shared_ptr<TestUserProcessor>;

    using TestDeleteWorkerProcessor = Processor<
            DeleteWorkerProcessorConfig,
            DeleteWorkerRequestProto,
            DeleteWorkerResponseProto,
            DeleteServiceProvider,
            TestRequestContext<DeleteWorkerResponseProto>,
            BufferView<DeleteWorkerRequestProto>,
            execute>;
    using TestDeleteWorkerProcessorS = std::shared_ptr<TestDeleteWorkerProcessor>;

    using TestSetupWorkerProcessor = Processor<
            SetupWorkerProcessorConfig,
            SetupWorkerRequestProto,
            SetupWorkerResponseProto,
            SetupServiceProvider,
            TestRequestContext<SetupWorkerResponseProto>,
            BufferView<SetupWorkerRequestProto>,
            execute>;
    using TestSetupWorkerProcessorS = std::shared_ptr<TestSetupWorkerProcessor>;

    template<typename TProcessor, typename TServiceProvider, typename TResponseProto>
    struct TestProcessorServices {
        std::unique_ptr<TProcessor> processor;
        std::shared_ptr<TServiceProvider> service_provider;
        std::unique_ptr<RequestContextWrapper<TResponseProto>> request_context_wrapper;
    };

    struct TestServices {
        BufferPoolS buffer_pool;
        storage::DatabaseManagerS database_manager;
        ThreadPoolS thread_pool;
        std::unique_ptr<TestProcessorServices<TestSetupWorkerProcessor, SetupServiceProvider, SetupWorkerResponseProto>> setup_worker;
        std::unique_ptr<TestProcessorServices<TestUserProcessor, UserServiceProvider, WorkerProcessUserResponseProto>> user;
        std::unique_ptr<TestProcessorServices<TestDeleteWorkerProcessor, DeleteServiceProvider, DeleteWorkerResponseProto>> delete_worker;
        virtual ~TestServices();
    };

    using TestServicesU = std::unique_ptr<TestServices>;

    Result<TestServicesU> setup_serenity_processors(const LogContext &log_context, WorkerId worker_id, bool user, bool setup_worker, bool delete_worker);

    namespace util {
        TestPackageU setup_worker_from_directory(const LogContext &log_context, TestServicesU &services, const std::string &directory, WorkerVersion previous_version);
    }

    struct ExpectedException {
        std::optional<std::string> message{};
        std::optional<std::string> stack{};
    };

    struct Context {
        std::shared_ptr<LogContext> log_context;
        TestServicesU services;
        TestPackageU package;
        fbs::Builder builder{};

        const WorkerProcessUserResponseProto *get_data(ClassId class_id, const PrimaryKey &primary_key, Code expected_code = Code::Ok) {
            auto req = test::create_get_object_request(*log_context, builder, package->worker_id, package->worker_version, class_id, primary_key);
            services->user->processor->post(std::move(req), services->user->request_context_wrapper->create_request_context());
            services->user->request_context_wrapper->wait(*log_context);
            const auto response = services->user->request_context_wrapper->get_result().get_payload();
            ESTATE_ASSERT(!response->deltas()); //no deltas
            ESTATE_ASSERT(!response->events()); //no events
            ESTATE_ASSERT(!response->console_log()); //no console messages
            if (expected_code == Code::Ok) {
                ESTATE_ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::GetDataResponseProto);
                ESTATE_ASSERT(response->response_nested_root()->value_as_GetDataResponseProto());
                ESTATE_ASSERT(response->response_nested_root()->value_as_GetDataResponseProto()->data());
            } else {
                ESTATE_ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::ErrorCodeResponseProto);
                const auto *proto = response->response_nested_root()->value_as_ErrorCodeResponseProto();
                ESTATE_ASSERT_EQ(proto->error_code(), GET_CODE_VALUE(expected_code));
            }
            return response;
        }

        const WorkerProcessUserResponseProto *call_service_method(ClassId class_id, const PrimaryKey &primary_key, MethodId method_id,
                                                             std::optional<std::vector<fbs::Offset<ValueProto>>> arguments,
                                                             std::optional<std::vector<fbs::Offset<InboundDataDeltaProto>>> referenced_data_deltas,
                                                             std::variant<Code, ExpectedException> expected = Code::Ok) {
            auto req = test::create_call_service_method_request(*log_context, builder, package->worker_id, package->worker_version, class_id,
                                                                primary_key, method_id, std::move(arguments),
                                                                std::move(referenced_data_deltas));
            services->user->processor->post(std::move(req), services->user->request_context_wrapper->create_request_context());
            services->user->request_context_wrapper->wait(*log_context);
            const auto response = services->user->request_context_wrapper->get_result().get_payload();

            if (response->console_log()) {
                const auto console = response->console_log_nested_root();
                if (console->messages() && console->messages()->size() > 0) {
                    for (auto i = 0; i < console->messages()->size(); ++i) {
                        const auto message = console->messages()->Get(i);
                        if(message->error()) {
                            log_trace(*log_context, "[TEST] <<console.error> {}", message->message()->str());
                        } else {
                            log_trace(*log_context, "[TEST] <<console.log> {}", message->message()->str());
                        }
                    }
                }
            }

            if (response->response_nested_root()->value_type() == UserResponseUnionProto::ExceptionResponseProto) {
                const auto ex = response->response_nested_root()->value_as_ExceptionResponseProto();
                log_trace(*log_context, "[TEST] Exception:\n Message: {}\n Stack: {}",
                          ex->message() ? ex->message()->str() : "<empty>",
                          ex->stack() ? ex->stack()->str() : "<empty>");
            }

            if (std::holds_alternative<ExpectedException>(expected)) {
                const auto ex = std::get<ExpectedException>(expected);
                const auto comp = response->response_nested_root()->value_as_ExceptionResponseProto();
                ESTATE_ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::ExceptionResponseProto);
                if(ex.message.has_value()) {
                    ESTATE_ASSERT(comp->message());
                    ESTATE_ASSERT_EQ(ex.message.value(), comp->message()->str());
                }
                if (ex.stack.has_value()) {
                    ESTATE_ASSERT(comp->stack());
                    ESTATE_ASSERT_EQ(ex.stack.value(), comp->stack()->str());
                }
            } else {
                const auto code = std::get<Code>(expected);
                if (code == Code::Ok) {
                    if(response->response_nested_root()->value_type() == UserResponseUnionProto::ErrorCodeResponseProto) {
                        log_trace(*log_context, "[TEST] Error code {} returned", get_code_name((Code) response->response_nested_root()->value_as_ErrorCodeResponseProto()->error_code()));
                    }
                    ESTATE_ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);
                } else {
                    ESTATE_ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::ErrorCodeResponseProto);
                    ESTATE_ASSERT_EQ(get_code_name((Code) response->response_nested_root()->value_as_ErrorCodeResponseProto()->error_code()), get_code_name(code));
                }
            }

            return response;
        }

        const WorkerProcessUserResponseProto *save_data(std::vector<fbs::Offset<InboundDataDeltaProto>> data_deltas, Code expected_code = Code::Ok) {
            auto req = test::create_save_data_request(*log_context, builder, package->worker_id, package->worker_version,
                                                              std::move(data_deltas));
            services->user->processor->post(std::move(req), services->user->request_context_wrapper->create_request_context());
            services->user->request_context_wrapper->wait(*log_context);
            const auto response = services->user->request_context_wrapper->get_result().get_payload();
            if (expected_code == Code::Ok) {
                ESTATE_ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::SaveDataResponseProto);
            } else {
                ESTATE_ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::ErrorCodeResponseProto);
                ESTATE_ASSERT_EQ(response->response_nested_root()->value_as_ErrorCodeResponseProto()->error_code(), (u16) expected_code);
            }
            return response;
        }
    };
}
