//
// Created by scott on 4/1/20.
//
#include "./estate_test.h"

#include <estate/internal/file_util.h>
#include <estate/runtime/version.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <utility>
#include <iostream>
#include <cstdlib>

namespace estate::test {
    std::string pad_right_string(const std::string &str, const size_t num, const char paddingChar) {
        std::string s{str};
        if (num > str.size())
            s.insert(s.end(), num - s.size(), paddingChar);
        return s;
    }
    namespace util {
        TestPackageU setup_worker_from_directory(const LogContext &log_context, TestServicesU &services,
                                               const std::string &directory, WorkerVersion previous_version) {
            auto package = load_package(services->buffer_pool, directory);

            fbs::Builder builder;
            auto request_buffer_view = create_setup_worker_request(log_context, builder, package->worker_id, package->worker_version, previous_version,
                                                                 std::move(package->worker_index), package->code);

            ESTATE_ASSERT_EQ(request_buffer_view.get_payload()->worker_id(), package->worker_id);
            ESTATE_ASSERT_EQ(request_buffer_view.get_payload()->worker_version(), package->worker_version);

            services->setup_worker->processor->post(std::move(request_buffer_view), services->setup_worker->request_context_wrapper->create_request_context());
            services->setup_worker->request_context_wrapper->wait(log_context);

            auto response = services->setup_worker->request_context_wrapper->get_result().get_payload();
            ESTATE_ASSERT(!response->error());

            auto db = services->database_manager->get_database(log_context, package->worker_id, false, std::nullopt).unwrap();

            auto worker_index_r = db->get_worker_index(log_context);
            ESTATE_ASSERT(worker_index_r);
            auto worker_index = worker_index_r.unwrap();
            ESTATE_ASSERT_EQ(worker_index.get_flatbuffer()->worker_id(), package->worker_id);
            ESTATE_ASSERT_EQ(worker_index.get_flatbuffer()->worker_version(), package->worker_version);

            auto engine_source_r = db->get_engine_source(log_context);
            ESTATE_ASSERT(engine_source_r);
            auto engine_source = engine_source_r.unwrap();
            ESTATE_ASSERT(engine_source->code_files() &&
                             engine_source->code_files()->size() == worker_index->file_names()->size() &&
                             package->code.size() == engine_source->code_files()->size());

            return std::move(package);
        }
    }

    std::vector<std::string> get_code_from_dir(const WorkerIndexProto &worker_index, std::string source_root_dir) {
        assert(worker_index.file_names());
        std::vector<std::string> code(worker_index.file_names()->size());
        for (auto i = 0; i < worker_index.file_names()->size(); ++i) {
            auto file = worker_index.file_names()->Get(i);
            auto name = file->file_name()->str();
            if (name.ends_with(".mjs")) {
                name.replace(name.end() - 4, name.end(), ".out.mjs");
            } else if (name.ends_with(".js")) {
                name.replace(name.end() - 3, name.end(), ".out.js");
            } else {
                throw std::domain_error("invalid file extension");
            }
            auto idx = file->file_name_id() - 1;
            auto path = source_root_dir + '/' + name;
            auto maybe_code_str = maybe_read_whole_binary_file(path);
            if (!maybe_code_str.has_value())
                throw std::domain_error("failed to read js file");
            code[idx] = std::move(maybe_code_str.value());
        }
        return std::move(code);
    }

    TestPackageU load_package(BufferPoolS buffer_pool, const std::string &package_dir) {
        auto worker_index_file = package_dir;
        worker_index_file += "/worker_index";

        auto maybe_bytes = maybe_read_whole_binary_file(worker_index_file);
        if (!maybe_bytes.has_value())
            throw std::domain_error("failed to read worker index");

        auto worker_index = buffer_pool->get_buffer<WorkerIndexProto>();
        worker_index.resize(maybe_bytes->size());
        std::memcpy(worker_index.as_u8(), maybe_bytes->data(), maybe_bytes->size());
        assert(worker_index->file_names());

        std::vector<std::string> code = get_code_from_dir(*worker_index.get_flatbuffer(), package_dir);

        auto package = std::make_unique<TestPackage>(worker_index->worker_id(), worker_index->worker_version(), std::move(worker_index), std::move(code));
        assert(package->worker_index.get_flatbuffer()->file_names());
        return std::move(package);
    }

    const BufferView<SetupWorkerRequestProto>
    create_setup_worker_request(const LogContext &log_context, fbs::Builder &builder, WorkerId worker_id, WorkerVersion worker_version,
                              WorkerVersion previous_worker_version, Buffer<WorkerIndexProto> &&worker_index, const std::vector<std::string> &code) {
        builder.Clear();

        std::vector<fbs::Offset<fbs::String>> code_vec{};
        for (auto &s : code) {
            code_vec.push_back(builder.CreateString(s));
        }

        std::vector<u8> worker_index_vec{worker_index.as_char(), worker_index.as_char() + worker_index.size()};

        auto proto = CreateSetupWorkerRequestProtoDirect(builder,
                                                       log_context.get_context().c_str(),
                                                       worker_id,
                                                       worker_version,
                                                       previous_worker_version,
                                                       &worker_index_vec,
                                                       &code_vec
        );

        builder.Finish(proto);

        return BufferView<SetupWorkerRequestProto>{builder};
    }

    const BufferView<DeleteWorkerRequestProto> create_delete_worker_request(const LogContext &log_context, fbs::Builder &builder, WorkerId worker_id, WorkerVersion worker_version) {
        builder.Clear();
        auto log_context_str = builder.CreateString(log_context.get_context());
        auto proto = CreateDeleteWorkerRequestProto(builder, log_context_str, worker_id, worker_version);
        builder.Finish(proto);
        return BufferView<DeleteWorkerRequestProto>{builder};
    }

    const BufferView<UserRequestProto> create_get_object_request(const LogContext &log_context, fbs::Builder &builder,
                                                                 WorkerId worker_id, WorkerVersion worker_version, ClassId class_id, const PrimaryKey &primary_key) {
        builder.Clear();
        std::string primary_key_str{primary_key.view()};
        auto proto = CreateUserRequestProtoDirect(builder, ESTATE_RIVER_PROTOCOL_VERSION, log_context.get_context().c_str(), worker_id, worker_version,
                                                  UserRequestUnionProto::GetDataRequestProto,
                                                  CreateGetDataRequestProtoDirect(builder, class_id, primary_key_str.c_str()).Union());
        builder.Finish(proto);
        return BufferView<UserRequestProto>{builder};
    }

    const BufferView<UserRequestProto>
    create_call_service_method_request(const LogContext &log_context, fbs::Builder &builder, WorkerId worker_id, WorkerVersion worker_version,
                                       ClassId class_id, const PrimaryKey &primary_key, MethodId method_id,
                                       std::optional<std::vector<fbs::Offset<ValueProto>>> arguments,
                                       std::optional<std::vector<fbs::Offset<InboundDataDeltaProto>>> referenced_data_detlas) {
        if (!arguments.has_value() && !referenced_data_detlas.has_value())
            builder.Clear();

        std::string primary_key_str{primary_key.view()};
        auto proto = CreateUserRequestProtoDirect(builder, ESTATE_RIVER_PROTOCOL_VERSION, log_context.get_context().c_str(), worker_id, worker_version,
                                                  UserRequestUnionProto::CallServiceMethodRequestProto,
                                                  CreateCallServiceMethodRequestProtoDirect(builder, class_id, primary_key_str.c_str(), method_id,
                                                                                                arguments.has_value() ? &arguments.value() : nullptr,
                                                                                                referenced_data_detlas.has_value() ? &referenced_data_detlas.value() : nullptr).Union());
        builder.Finish(proto);
        return BufferView<UserRequestProto>{builder};
    }
    const BufferView<UserRequestProto>
    create_save_data_request(const LogContext &log_context, fbs::Builder &builder, WorkerId worker_id, WorkerVersion worker_version,
                                     std::vector<fbs::Offset<InboundDataDeltaProto>> data) {
        auto proto = CreateUserRequestProtoDirect(builder, ESTATE_RIVER_PROTOCOL_VERSION, log_context.get_context().c_str(), worker_id, worker_version,
                                                  UserRequestUnionProto::SaveDataRequestProto,
                                                  CreateSaveDataRequestProtoDirect(builder, &data).Union());
        builder.Finish(proto);
        return BufferView<UserRequestProto>{builder};
    }

    Result<TestServicesU> setup_serenity_processors(const LogContext &log_context, WorkerId worker_id, bool user, bool setup_worker, bool delete_worker) {
        using Result = Result<TestServicesU>;
        auto base_path = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();

        log_trace(log_context, "Using the database directory {}", base_path.string());

        if (!boost::filesystem::create_directories(base_path)) {
            log_critical(log_context, "Unable to Create directories at {}", base_path.string());
            return Result::Error();
        }

        std::string wal_dir_fmt(base_path.string());
        wal_dir_fmt.append("/{0}/wal");

        std::string data_dir_fmt(base_path.string());
        data_dir_fmt.append("/{0}/data");

        std::string delete_file_fmt(base_path.string());
        delete_file_fmt.append("/{0}/deleted");

        storage::DatabaseManagerConfiguration config{
                wal_dir_fmt, data_dir_fmt, delete_file_fmt, true
        };

        auto test_services = std::make_unique<TestServices>();

        BufferPoolConfig buffer_pool_config{true};

        auto buffer_pool = std::make_shared<BufferPool>(buffer_pool_config);
        test_services->buffer_pool = buffer_pool;

        test_services->database_manager = std::make_shared<storage::DatabaseManager>(config, buffer_pool);

        if (!engine::javascript::is_initialized()) {
            engine::javascript::JavascriptConfig platform_config{
                    "", ""
            };
            engine::javascript::initialize(platform_config);
        }

        ThreadPoolConfig thread_pool_config{};
        test_services->thread_pool = std::make_shared<ThreadPool>(thread_pool_config);

        engine::IObjectRuntimeS object_runtime{};

        if (user) {
            object_runtime = engine::javascript::create_object_runtime(10485760);
            auto cm = std::make_unique<TestProcessorServices<TestUserProcessor, UserServiceProvider, WorkerProcessUserResponseProto>>();
            auto sp = std::make_shared<UserServiceProvider>(test_services->buffer_pool, test_services->database_manager, object_runtime);
            cm->processor = std::make_unique<TestUserProcessor>(UserProcessorConfig::Create(worker_id), sp);
            cm->service_provider = sp;
            cm->request_context_wrapper = std::make_unique<RequestContextWrapper<WorkerProcessUserResponseProto>>();
            test_services->user = std::move(cm);
        }

        if (setup_worker) {
            auto sw = std::make_unique<TestProcessorServices<TestSetupWorkerProcessor, SetupServiceProvider, SetupWorkerResponseProto>>();
            auto js_runtime = engine::javascript::create_setup_runtime();
            auto sp = std::make_shared<SetupServiceProvider>(test_services->buffer_pool, test_services->database_manager, js_runtime);
            sw->processor = std::make_unique<TestSetupWorkerProcessor>(SetupWorkerProcessorConfig::Create(worker_id), sp);
            sw->service_provider = sp;
            sw->request_context_wrapper = std::make_unique<RequestContextWrapper<SetupWorkerResponseProto>>();
            test_services->setup_worker = std::move(sw);
        }

        if (delete_worker) {
            auto dw = std::make_unique<TestProcessorServices<TestDeleteWorkerProcessor, DeleteServiceProvider, DeleteWorkerResponseProto>>();


            auto sp = std::make_shared<DeleteServiceProvider>(test_services->buffer_pool,
                                                                  test_services->database_manager,
                                                                  std::nullopt,
                                                                  std::nullopt);
            dw->processor = std::make_unique<TestDeleteWorkerProcessor>(DeleteWorkerProcessorConfig::Create(worker_id, false), sp);
            dw->service_provider = sp;
            dw->request_context_wrapper = std::make_unique<RequestContextWrapper<DeleteWorkerResponseProto>>();
            test_services->delete_worker = std::move(dw);
        }

        test_services->thread_pool->start();

        return Result::Ok(std::move(test_services));
    }

    std::string get_test_data_directory(const std::string &test_suite, const std::string &test_name) {
        const auto val = std::getenv("ESTATE_CPP_GENERATED_TEST_DATA_DIR");
        if(!val)
            throw std::domain_error("Missing ESTATE_CPP_GENERATED_TEST_DATA_DIR environment variable");
        const std::string val_str{val};
        auto rel_dir = fmt::format("{0}/{1}/{2}", val_str, test_suite, test_name);
        auto dir = boost::filesystem::canonical(rel_dir).string();
        std::cout << "Using test directory " << dir << std::endl;
        return dir;
    }

    std::string gen_random_string(const int len) {
        using namespace boost::uuids;
        static random_generator random_generator{};
        uuid u = random_generator();

        std::string result;
        result.reserve(len);

        std::size_t i = 0;
        for (uuid::const_iterator it_data = u.begin(); it_data != u.end() && i < (len / 2); ++it_data, ++i) {
            const size_t hi = ((*it_data) >> 4) & 0x0F;
            result += boost::uuids::detail::to_char(hi);
            const size_t lo = (*it_data) & 0x0F;
            result += boost::uuids::detail::to_char(lo);
        }

        return result;
    }

    TestServices::~TestServices() {
        thread_pool->shutdown();
        thread_pool = nullptr;
    }
    TestPackage::TestPackage(WorkerId worker_id, WorkerVersion worker_version, Buffer<WorkerIndexProto> worker_index, std::vector<std::string> code) :
            worker_id(worker_id), worker_version(worker_version), worker_index(std::move(worker_index)),
            code(std::move(code)) {
    }
}
