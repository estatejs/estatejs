//
// Created by scott on 2/11/20.
//
#pragma once

#include "server_fwd.h"
#include "server_decl.h"
#include "v8_macro.h"

#include "estate/internal/pool.h"
#include "estate/internal/buffer_pool.h"
#include "estate/internal/logging.h"
#include "estate/internal/local_config.h"
#include "estate/internal/flatbuffers_util.h"
#include "estate/internal/stopwatch.h"
#include "estate/internal/processor/service.h"
#include "estate/internal/deps/rocksdb.h"
#include "estate/internal/deps/boost.h"
#include "estate/internal/deps/v8.h"

#include <estate/runtime/protocol/WorkerIndexProto_generated.h>
#include <estate/runtime/protocol/worker_process_engine_model_generated.h>
#include <estate/runtime/protocol/worker_process_interface_generated.h>

#include <estate/runtime/numeric_types.h>
#include <estate/runtime/model_types.h>
#include <estate/runtime/blob.h>
#include <estate/runtime/buffer_view.h>
#include <estate/runtime/code.h>
#include <estate/runtime/result.h>

#include <utility>
#include <ostream>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <memory>
#include <vector>
#include <mutex>

namespace estate {
    namespace data {
        using Cell = Buffer<CellProto>;
        using CellView = BufferView<CellProto>;

        enum class ClassType : u8 {
            WORKER_OBJECT = 0,
            WORKER_SERVICE = 1,
            WORKER_EVENT = 2
        };
        struct ClassInfo {
            ClassType class_type;
            ClassId class_id;
        };
        enum class ObjectType : u8 {
            WORKER_OBJECT = 0,
            WORKER_SERVICE = 1
        };
        template<ObjectType OT>
        constexpr const ClassType get_class_type() {
            return (ClassType) OT;
        }
        //Class that references an object that MAY exist in the ObjectTable.
        //May be persisted. Must be resolved.
        class ObjectReference {
        private:
            std::optional<std::string> _object_instance_key;
            std::optional<std::string> _object_properties_index_key;
            PrimaryKey _primary_key;
            bool _moved;
            std::optional<size_t> _hash_code;
        public:
            const ObjectType type;
            const ClassId class_id;
        public:
            [[nodiscard]] bool is_service() const;
            [[nodiscard]] bool is_data() const;
            [[nodiscard]] const PrimaryKey &get_primary_key() const;
            std::string_view get_object_instance_key();
            std::string_view get_object_properties_index_key();
            ObjectReference(ObjectType type, ClassId class_id, PrimaryKey primary_key);
            ObjectReference(ObjectReference &&other) noexcept;
            bool operator==(const ObjectReference &rhs) const;
            bool operator!=(const ObjectReference &rhs) const;
        };
        ObjectReferenceS make_object_reference(ObjectType type, ClassId class_id, PrimaryKey primary_key);

        //Class that references an object that DOES exist in the ObjectTable.
        //Must not be persisted. Invalid outside a transaction.
        class ObjectHandle {
            const ObjectReferenceS _reference;
            const ObjectVersion _original_version; //the version of the object when it was first loaded.
            ObjectVersion _version; //authoritative object version.
        private:
            ObjectHandle(const ObjectHandle &) = delete;
            ObjectHandle(ObjectHandle &&) = delete;
        public:
            ObjectHandle(ObjectReferenceS object_reference, ObjectVersion original_version, ObjectVersion version);
            [[nodiscard]] bool is_data() const;
            [[nodiscard]] bool is_service() const;
            [[nodiscard]] ObjectReferenceS get_reference() const;
            [[nodiscard]] ObjectVersion get_original_version() const;
            [[nodiscard]] ObjectVersion get_version() const;
            // Makes the version one more than the original version.
            // This means the object can be written to the database any number of times within the same transaction
            // but only have its version number increment once.
            // Note: This does not effect the hash_code.
            void increment_version();
            bool operator==(const ObjectHandle &rhs) const;
            bool operator!=(const ObjectHandle &rhs) const;
        };
        ObjectHandleS make_object_handle(ObjectReferenceS object_reference, ObjectVersion original_version, ObjectVersion version);

        class Tracker;
        using TrackerS = std::shared_ptr<Tracker>;

        ResultCode<fbs::Offset<fbs::Vector<fbs::Offset<NestedDataProto>>>> export_data(
                fbs::Builder &target_builder, const data::ObjectReferenceS &start, WorkingSetS working_set);
        ResultCode<std::optional<data::ObjectHandleS>> apply_inbound_delta(
                fbs::Builder &reusable_builder, const InboundDataDeltaProto &delta, WorkingSetS working_set, BufferPoolS buffer_pool, bool save);
        std::vector<Buffer<DataDeltaProto>> export_deltas_from_working_set(const LogContext & log_context, BufferPoolS buffer_pool, fbs::BuilderS reusabled_builder,
                                                                WorkingSetS working_set);
        std::vector<std::pair<ObjectHandleS, Buffer<DataDeltaProto>>> create_deltas(const LogContext &log_context,
                                                                                          BufferPoolS buffer_pool,
                                                                                          fbs::BuilderS reusable_builder,
                                                                                          std::vector<data::ObjectS> &objects);
    }
    namespace storage {
        struct ITransaction : public virtual std::enable_shared_from_this<ITransaction> {
            [[nodiscard]] virtual ResultCode<std::optional<data::Cell>> maybe_get_cell(const std::string &property_key) = 0;
            [[nodiscard]] virtual ResultCode<Buffer<WorkerIndexProto>, Code> get_worker_index() = 0;
            [[nodiscard]] virtual ResultCode<Buffer<EngineSourceProto>, Code> get_engine_source() = 0;
            [[nodiscard]] virtual WorkerId get_worker_id() = 0;
            [[nodiscard]] virtual WorkerVersion get_worker_version() = 0;
            [[nodiscard]] virtual UnitResultCode delete_worker_index() = 0;
            [[nodiscard]] virtual UnitResultCode delete_engine_source() = 0;
            [[nodiscard]] virtual UnitResultCode save_worker_index(WorkerVersion new_worker_version, const BufferView<WorkerIndexProto> &worker_index) = 0;
            [[nodiscard]] virtual UnitResultCode save_engine_source(BufferView<EngineSourceProto> engine_source) = 0;
            [[nodiscard]] virtual UnitResultCode commit() = 0;
            [[nodiscard]] virtual UnitResultCode write_cell(const data::CellView &cell_buffer, const std::string_view key) = 0;
            virtual ~ITransaction() = default;
            [[nodiscard]] virtual UnitResultCode delete_value(const std::string_view key) = 0;
            [[nodiscard]] virtual ResultCode<bool> object_instance_exists(const data::ObjectReferenceS &ref) = 0;
            [[nodiscard]] virtual ResultCode<std::optional<Buffer<ObjectInstanceProto>>> maybe_get_object_instance(const data::ObjectReferenceS &ref) = 0;
            [[nodiscard]] virtual ResultCode<std::optional<Buffer<ObjectPropertiesIndexProto>>> maybe_get_object_properties_index(const data::ObjectReferenceS &ref) = 0;
            [[nodiscard]] virtual UnitResultCode write_object_instance(const data::ObjectReferenceS &ref, ObjectVersion version, bool deleted) = 0;
            [[nodiscard]] virtual UnitResultCode write_object_properties_index(const data::ObjectReferenceS &ref, const std::optional<std::set<std::string>> &property_names) = 0;
            virtual void undo_get_for_update(const std::string_view key) = 0;
        };

        struct IDatabase {
            [[maybe_unused]] virtual ResultCode<WorkerVersion, Code> get_worker_version(const LogContext &log_context) = 0;
            [[nodiscard]] virtual ResultCode<ITransactionS, Code> create_transaction(const LogContext &log_context, WorkerVersion worker_version) = 0;
            [[nodiscard]] virtual ResultCode<Buffer<WorkerIndexProto>, Code> get_worker_index(const LogContext &log_context) = 0;
            [[nodiscard]] virtual ResultCode<Buffer<EngineSourceProto>, Code> get_engine_source(const LogContext &log_context) = 0;
            [[nodiscard]] virtual UnitResultCode mark_as_deleted(const LogContext &log_context) = 0;
            virtual ~IDatabase() = default;
        };

        struct DatabaseManagerConfiguration {
            std::string wal_dir_format;
            std::string data_dir_format;
            std::string deleted_file_format;
            bool optimize_for_small_db;
            static DatabaseManagerConfiguration FromRemote(const LocalConfigurationReader &reader) {
                return DatabaseManagerConfiguration{
                        reader.get_string("wal_dir_format"),
                        reader.get_string("data_dir_format"),
                        reader.get_string("deleted_file_format"),
                        reader.get_bool("optimize_for_small_db")
                };
            }
        };

        class DatabaseManager {
            const DatabaseManagerConfiguration config;
            std::mutex databases_mutex;
            std::unordered_map<WorkerId, IDatabaseS> databases;
            std::mutex open_databases_mutexes_mutex;
            std::unordered_map<WorkerId, std::mutex> open_database_mutexes;
            Service<BufferPool> buffer_pool_service;
        public:
            const DatabaseManagerConfiguration &get_config();
            explicit DatabaseManager(DatabaseManagerConfiguration config, BufferPoolS buffer_pool);
            [[nodiscard]] ResultCode<IDatabaseS, Code> get_database(const LogContext &log_context, WorkerId worker_id, bool is_new, std::optional<WorkerVersion> initial_worker_version);
            //NOTE: this doesn't close the database immediately. That won't happen until the last database reference is deleted.
            void close_database(const LogContext &log_context, WorkerId worker_id);
        private:
            [[nodiscard]] Result<IDatabaseS> try_get_database(WorkerId worker_id);
            [[nodiscard]] ResultCode<IDatabaseS, Code> open_database(const LogContext &log_context, WorkerId worker_id, bool is_new, std::optional<WorkerVersion> initial_worker_version);
            [[nodiscard]] std::mutex *get_and_lock_open_databases_mutex(WorkerId worker_id);
        };
    }
    namespace engine {
        struct ScriptException {
            std::optional<std::string> message;
            std::optional<std::string> stack_trace;
        };

        class EngineError {
            const std::variant<Code, ScriptException> _error;
            using ErrorVariant = std::variant<Code, ScriptException>;
            EngineError(ErrorVariant c);
        public:
            EngineError(Code c);
            EngineError(ScriptException ex);
            constexpr enum Code get_code() const {
                if (!is_code()) {
                    assert(false);
                    throw std::domain_error("attempted to get_for_read code when exception present");
                }
                return std::get<0>(_error);
            }
            constexpr const ScriptException &get_exception() const {
                if (!is_exception()) {
                    assert(false);
                    throw std::domain_error("attempted to get_for_read exception when code present");
                }
                return std::get<1>(_error);
            }
            constexpr bool is_code() const {
                return _error.index() == 0;
            }
            constexpr bool is_exception() const {
                return _error.index() == 1;
            }
        };

        class ConsoleLog {
            std::optional<std::vector<std::pair<std::string, bool>>> _maybe_messages{};
        public:
            ConsoleLog() = default;
            ConsoleLog(ConsoleLog &&) = delete;
            ConsoleLog(const ConsoleLog &) = delete;
            void append_log(const std::string message);
            void append_error(const std::string message);
            const std::optional<std::vector<std::pair<std::string, bool>>> &maybe_get_messages();
        };
        using ConsoleLogS = std::shared_ptr<ConsoleLog>;
        std::optional<Buffer<ConsoleLogProto>> create_console_log_proto(BufferPoolS buffer_pool, ConsoleLogS console_log);

        class CallContext : public virtual std::enable_shared_from_this<CallContext> {
            const LogContext &_log_context;
            storage::ITransactionS _txn;
            BufferPoolS _buffer_pool;
            const bool _cached_working_set;

            std::optional<data::WorkingSetS> _maybe_working_set;
            std::optional<fbs::BuilderS> _maybe_reusable_builder;
            std::optional<ConsoleLogS> _maybe_console_log;
            std::optional<std::vector<Buffer<DataDeltaProto>>> _maybe_deltas{};
            std::optional<std::vector<Buffer<MessageProto>>> _maybe_fired_events{};
            std::optional<v8::Isolate *> _maybe_isolate{};
        public:
            CallContext(const LogContext &log_context, storage::ITransactionS txn, BufferPoolS buffer_pool, bool cached_working_set);
            virtual ~CallContext();
            [[nodiscard]] storage::ITransactionS get_transaction() const;
            [[nodiscard]] data::WorkingSetS get_working_set();
            [[nodiscard]] std::optional<std::vector<Buffer<DataDeltaProto>>> extract_deltas();
            void add_delta(Buffer<DataDeltaProto> delta);
            [[nodiscard]] std::optional<std::vector<Buffer<MessageProto>>> extract_fired_events();
            void add_fired_event(Buffer<MessageProto> event);
            [[nodiscard]] const LogContext &get_log_context() const;
            [[nodiscard]] fbs::BuilderS get_reusable_builder(bool clear);
            [[nodiscard]] BufferPoolS get_buffer_pool() const;
            [[nodiscard]] ConsoleLogS get_console_log();
            [[nodiscard]] v8::Isolate *get_isolate() const;
            void set_isolate(v8::Isolate *isolate);
        };

        struct CallServiceMethodResult {
            bool has_changes;
            Buffer<WorkerProcessUserResponseProto> response;
        };

        struct IObjectRuntime {
            [[nodiscard]] virtual EngineResultCode<CallServiceMethodResult> call_service_method(CallContextS call_context, const CallServiceMethodRequestProto *request) = 0;
        };

        struct ISetupRuntime {
            [[nodiscard]] virtual UnitEngineResultCode setup(const LogContext &log_context, storage::ITransactionS txn,
                                                             const SetupWorkerRequestProto* request, bool is_new) = 0;
        };

        namespace javascript {
            struct JavascriptConfig {
                std::string icu_location{};
                std::string external_startup_data{};
                size_t max_heap_size;
                static JavascriptConfig FromRemote(const LocalConfigurationReader &reader) {
                    return JavascriptConfig{
                            reader.get_string("icu_location", ""),
                            reader.get_string("external_startup_data", ""),
                            reader.get_u64("max_heap_size")
                    };
                }
            };
            bool is_initialized();
            void initialize(const JavascriptConfig &options);
            void shutdown();
            ISetupRuntimeS create_setup_runtime();
            IObjectRuntimeS create_object_runtime(size_t max_heap_size);
            enum class ClassLookupCode : u8 {
                WRONG_CLASS_TYPE = 0,
                NOT_FOUND = 1
            };
            ScriptException create_script_exception(const LogContext& log_context, v8::Isolate *isolate_, const v8::TryCatch &try_catch);
            void log_script_exception(const LogContext &log_context, const ScriptException &ex);
            std::string value_to_string(v8::Isolate *isolate, const v8::Local<v8::Value> &value);
            ResultCode<fbs::Offset<ValueProto>> serialize(const LogContext &log_context, v8::Isolate *isolate_, fbs::Builder &builder,
                                                                  const v8::Local<v8::Value> value, ValueUnionProto &type,
                                                                  std::optional<data::TrackerS> maybe_tracker);
            EngineResultCode<v8::Local<v8::Value>> deserialize(const LogContext &log_context, storage::ITransactionS txn, v8::Isolate *isolate_, const ValueProto *proto);
            namespace native {
                void noop(const v8::FunctionCallbackInfo<v8::Value> &args);
                namespace runtime {
                    void ESTATE_NEW_MESSAGE_CTOR(const v8::FunctionCallbackInfo<v8::Value> &args);
                    void ESTATE_NEW_DATA_CTOR(const v8::FunctionCallbackInfo<v8::Value> &args);
                    void ESTATE_NEW_SERVICE_CTOR(const v8::FunctionCallbackInfo<v8::Value> &args);
                    void ESTATE_CREATE_UUID_FUNCTION(const v8::FunctionCallbackInfo<v8::Value> &args);
                    namespace system {
                        void ESTATE_GET_SERVICE_FUNCTION(const v8::FunctionCallbackInfo<v8::Value> &args);
                        void ESTATE_REVERT_FUNCTION(const v8::FunctionCallbackInfo<v8::Value> &args);
                        void ESTATE_GET_DATA_FUNCTION(const v8::FunctionCallbackInfo<v8::Value> &args);
                        void ESTATE_DELETE_FUNCTION(const v8::FunctionCallbackInfo<v8::Value> &args);
                        void ESTATE_SAVE_DATA_GRAPHS_FUNCTION(const v8::FunctionCallbackInfo<v8::Value> &args);
                        void ESTATE_SAVE_DATA_FUNCTION(const v8::FunctionCallbackInfo<v8::Value> &args);
                        void ESTATE_SEND_MESSAGE_FUNCTION(const v8::FunctionCallbackInfo<v8::Value> &args);
                    }
                }
                namespace runtime_internal {
                    void ESTATE_EXISTING_DATA_CTOR(const v8::FunctionCallbackInfo<v8::Value> &args);
                    void ESTATE_EXISTING_SERVICE_CTOR(const v8::FunctionCallbackInfo<v8::Value> &args);
                }
                namespace console {
                    void ESTATE_CONSOLE_LOG_FUNCTION(const v8::FunctionCallbackInfo<v8::Value> &args);
                    void ESTATE_CONSOLE_ERROR_FUNCTION(const v8::FunctionCallbackInfo<v8::Value> &args);
                }
                namespace object { //note: these don't need constant names because they're not called by name.
                    void on_service_property_get(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value> &info);
                    void on_service_property_set(v8::Local<v8::Name> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value> &info);
                    void on_service_property_delete(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Boolean> &info);
                    void on_object_property_get(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value> &info);
                    void on_object_property_set(v8::Local<v8::Name> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value> &info);
                    void on_object_property_delete(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Boolean> &info);
                }
            }
        }
    }
}
