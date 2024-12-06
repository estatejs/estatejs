//
// Created by scott on 2/11/20.
//

#include "estate/internal/server/server.h"
#include "estate/internal/server/server_decl.h"
#include "estate/internal/server/v8_macro.h"

#include "estate/internal/database_keys.h"
#include "estate/internal/logging.h"
#include "estate/internal/pool.h"
#include "estate/runtime/limits.h"
#include "estate/internal/flatbuffers_util.h"
#include "estate/internal/stopwatch.h"
#include "estate/internal/buffer_pool.h"
#include "estate/internal/file_util.h"
#include "estate/internal/deps/boost.h"

#include <estate/runtime/buffer_view.h>
#include <estate/runtime/model_types.h>
#include <estate/runtime/code.h>
#include <estate/runtime/result.h>

#include <cassert>
#include <iostream>
#include <utility>
#include <filesystem>

#include "server_rc.inl"

namespace estate {
    namespace data {
        void _get_crc(boost::crc_32_type &crc, const ValueProto *value) {
            const auto type = value->value_type();

            crc.process_byte((u8) type);

            switch (type) {
                case ValueUnionProto::UndefinedValueProto: {
                    break;
                }
                case ValueUnionProto::NullValueProto: {
                    break;
                }
                case ValueUnionProto::BooleanValueProto: {
                    const auto v = value->value_as_BooleanValueProto()->value();
                    crc.process_bytes(&v, sizeof(decltype(v)));
                    break;
                }
                case ValueUnionProto::StringValueProto: {
                    const auto sv = value->value_as_StringValueProto()->value()->string_view();
                    crc.process_bytes(sv.data(), sv.size());
                    break;
                }
                case ValueUnionProto::NumberValueProto: {
                    const auto v = value->value_as_NumberValueProto()->value();
                    crc.process_bytes(&v, sizeof(decltype(v)));
                    break;
                }
                case ValueUnionProto::ObjectValueProto: {
                    const auto properties = value->value_as_ObjectValueProto()->properties();
                    if (properties) {
                        {
                            const auto s = properties->size();
                            crc.process_bytes(&s, sizeof(decltype(s)));
                        }
                        for (auto i = 0; i < properties->size(); ++i) {
                            const auto property = properties->Get(i);
                            const auto name = property->name()->string_view();
                            crc.process_bytes(name.data(), name.size());
                            _get_crc(crc, property->value());
                        }
                    }
                    break;
                }
                case ValueUnionProto::ArrayValueProto: {
                    const auto items = value->value_as_ArrayValueProto()->items();
                    if (items) {
                        {
                            const auto s = items->size();
                            crc.process_bytes(&s, sizeof(decltype(s)));
                        }
                        for (auto i = 0; i < items->size(); ++i) {
                            const auto item = items->Get(i);
                            const auto index = item->index();
                            crc.process_bytes(&index, sizeof(decltype(index))); //note: arrays are sparce.
                            _get_crc(crc, item->value());
                        }
                    }
                    break;
                }
                case ValueUnionProto::DataReferenceValueProto: {
                    const auto ref = value->value_as_DataReferenceValueProto();
                    const auto pksv = ref->primary_key()->string_view();
                    crc.process_bytes(pksv.data(), pksv.size());
                    const auto class_id = ref->class_id();
                    crc.process_bytes(&class_id, sizeof(decltype(class_id)));
                    break;
                }
                case ValueUnionProto::ServiceReferenceValueProto: {
                    const auto ref = value->value_as_ServiceReferenceValueProto();
                    const auto pksv = ref->primary_key()->string_view();
                    crc.process_bytes(pksv.data(), pksv.size());
                    const auto class_id = ref->class_id();
                    crc.process_bytes(&class_id, sizeof(decltype(class_id)));
                    break;
                }
                case ValueUnionProto::MapValueProto: {
                    const auto items = value->value_as_MapValueProto()->items();
                    if (items) {
                        {
                            const auto s = items->size();
                            crc.process_bytes(&s, sizeof(decltype(s)));
                        }
                        for (auto i = 0; i < items->size(); ++i) {
                            const auto item = items->Get(i);
                            _get_crc(crc, item->key());
                            _get_crc(crc, item->value());
                        }
                    }
                    break;
                }
                case ValueUnionProto::SetValueProto: {
                    const auto items = value->value_as_SetValueProto()->items();
                    if (items) {
                        {
                            const auto s = items->size();
                            crc.process_bytes(&s, sizeof(decltype(s)));
                        }
                        for (auto i = 0; i < items->size(); ++i) {
                            _get_crc(crc, items->Get(i));
                        }
                    }
                    break;
                }
                case ValueUnionProto::DateValueProto: {
                    const auto v = value->value_as_DateValueProto()->value();
                    crc.process_bytes(&v, sizeof(decltype(v)));
                    break;
                }
                default: {
                    assert(false);
                    break;
                }
            }
        }
        u32 get_crc(const ValueProto *value) {
            boost::crc_32_type crc;
            _get_crc(crc, value);
            return crc.checksum();
        }

        struct ObjectReferenceWrapper {
            const ObjectReferenceS ref;
            struct Hasher {
                std::size_t operator()(const ObjectReferenceWrapper &wrapper) const {
                    using boost::hash_value;
                    using boost::hash_combine;
                    std::size_t seed = 0;
                    hash_combine(seed, hash_value(wrapper.ref->type));
                    hash_combine(seed, hash_value(wrapper.ref->class_id));
                    hash_combine(seed, hash_value(wrapper.ref->get_primary_key().view()));
                    return seed;
                }
            };
        };
        bool operator==(const ObjectReferenceWrapper &lhs, const ObjectReferenceWrapper &rhs) {
            return *lhs.ref == *rhs.ref;
        }

        struct ObjectHandleWrapper {
            const ObjectHandleS handle;
            struct Hasher {
                std::size_t operator()(const ObjectHandleWrapper &wrapper) const {
                    using boost::hash_value;
                    using boost::hash_combine;
                    ObjectReferenceWrapper::Hasher ref_hasher{};
                    std::size_t seed = ref_hasher(ObjectReferenceWrapper{wrapper.handle->get_reference()});
                    hash_combine(seed, hash_value(wrapper.handle->get_original_version()));
                    return seed;
                }
            };
        };
        bool operator==(const ObjectHandleWrapper &lhs, const ObjectHandleWrapper &rhs) {
            return *lhs.handle == *rhs.handle;
        }

        using HandleWrapperSet = std::unordered_set<ObjectHandleWrapper, ObjectHandleWrapper::Hasher>;
        class Tracker {
            HandleWrapperSet _handles{};
            std::vector<ObjectHandleS> _new_handles{};
        public:
            static TrackerS Create() {
                return std::make_shared<Tracker>();
            }
            void track(ObjectHandleS handle) {
                assert(handle->get_reference()->type == ObjectType::WORKER_OBJECT);
                const auto &[_, inserted] = _handles.insert(ObjectHandleWrapper{handle});
                if (inserted) {
                    _new_handles.push_back(handle);
                }
            }
            [[nodiscard]] const HandleWrapperSet &all_handles() const {
                return _handles;
            }
            [[nodiscard]] std::vector<ObjectHandleS> extract_new_handles() {
                auto new_handles = std::move(_new_handles);
                _new_handles.clear();
                return std::move(new_handles);
            }
            [[nodiscard]] std::size_t all_handle_count() const {
                return _handles.size();
            }
            [[nodiscard]] std::size_t new_handle_count() const {
                return _new_handles.size();
            }
        };

        class SparceCell {
            std::optional<std::variant<Cell, CellView>> _maybe_variant_cell{};
        public:
            SparceCell(std::optional<std::variant<Cell, CellView>> maybe_variant_cell) :
                    _maybe_variant_cell{std::move(maybe_variant_cell)} {
            }
            SparceCell(const SparceCell &) = delete;
            SparceCell(SparceCell &&) = delete;
            [[nodiscard]] constexpr bool exists() const {
                return _maybe_variant_cell.has_value();
            }
            [[nodiscard]] constexpr bool is_cell() const {
                return !is_view();
            }
            // - Must not call when exists() == false
            [[nodiscard]] constexpr bool is_view() const {
                return std::holds_alternative<CellView>(*_maybe_variant_cell);
            }
            [[nodiscard]] Cell extract() {
                assert(exists());
                assert(is_cell());
                auto cell = std::move(std::get<0>(*_maybe_variant_cell));
                _maybe_variant_cell.reset();
                return std::move(cell);
            }
            // - Must not call when exists() == false
            [[nodiscard]] const CellView get_view() const {
                assert(exists());
                if (std::holds_alternative<Cell>(*_maybe_variant_cell)) {
                    return std::get<0>(*_maybe_variant_cell).get_view();
                } else {
                    return std::get<1>(*_maybe_variant_cell);
                }
            }
            // Returns whether it existed previously.
            bool erase() {
                bool existed_previously = exists();
                _maybe_variant_cell.reset();
                return existed_previously;
            }
            void set(std::variant<Cell, CellView> maybe_variant_cell) {
                _maybe_variant_cell.emplace(std::move(maybe_variant_cell));
            }
        };

        class CellState {
            SparceCell _saved;
            SparceCell _current;
        public:
            // New
            CellState() :
                    _saved{std::nullopt}, _current{std::nullopt} {}
            // Existing
            CellState(Cell saved) :
                    _saved{std::move(saved)}, _current{_saved.get_view()} {}
            CellState(const CellState &) = delete;
            CellState(CellState &&) = delete;
            constexpr bool saved_exists() const {
                return _saved.exists();
            }
            constexpr bool current_exists() const {
                return _current.exists();
            }
            // - Must not call when saved_exists() == false
            [[nodiscard]] const CellView get_saved() const {
                assert(saved_exists());
                return _saved.get_view();
            }
            // - Must not call when current_exists() == false
            [[nodiscard]] const CellView get_current() const {
                assert(current_exists());
                return _current.get_view();
            }
            // Sets the current cell's value.
            void set_current(Cell cell) {
                // if saved exists and it's a view then move the cell in current to saved
                // replacing saved's view with the actual cell.
                if (_saved.exists() && _saved.is_view()) {
                    assert(_current.exists()); //should exist since saved is a view to it.
                    assert(_current.is_cell()); //should be a cell since saved is a view to it.
                    _saved.set(std::move(_current.extract()));
                }
                _current.set(std::move(cell));
            }
            bool erase_current() {
                if (!_current.exists())
                    return false;

                if (_saved.exists() && _saved.is_view()) {
                    assert(_current.exists()); //should exist since saved is a view to it.
                    assert(_current.is_cell()); //should be a cell since saved is a view to it.
                    //note: this erases the current cell
                    _saved.set(std::move(_current.extract()));
                    return true;
                } else {
                    return _current.erase();
                }
            }
            // Applies the current state to saved.
            void apply_current_to_saved() {
                if (!_current.exists()) {
                    _saved.erase();
                } else if (_current.is_cell()) {
                    _saved.set(_current.get_view());
                }
                //do nothing if current is a view because it's already equal to saved.
            }
            // Applies the saved state to current.
            void apply_saved_to_current() {
                if (!_saved.exists()) {
                    _current.erase();
                } else if (_saved.is_cell()) {
                    _current.set(_saved.get_view());
                }
                //do nothing if saved is a view because it's already equal to current.
            }
        };

        class ScriptValue {
            std::optional<v8::Global<v8::Value>> _maybe_script_value{};
        public:
            // No preconditions.
            void set_unknown() {
                _maybe_script_value.reset();
            }
            // No preconditions.
            // Returns whether the value existed previously.
            bool erase() {
                bool previously_existed = known() && exists();
                _maybe_script_value.emplace(); //sets Global to IsEmpty so known() == true
                return previously_existed;
            }
            // No preconditions.
            [[nodiscard]] constexpr bool known() const {
                return _maybe_script_value.has_value();
            }
            // - Must not call when known() ==  false
            [[nodiscard]] bool exists() const {
                assert(known());
                return !_maybe_script_value->IsEmpty();
            }
            // Loads the current value into the script value.
            // No preconditions.
            [[nodiscard]] engine::UnitEngineResultCode apply(engine::CallContextS call_context, const CurrentValue &current);

            // - Must not call when known() == false
            [[nodiscard]] v8::Local<v8::Value> get_script_value(engine::CallContextS call_context) const {
                using Result = engine::EngineResultCode<v8::Local<v8::Value>>;
                assert(known());

                V8_ESCAPABLE_SCOPE(call_context->get_isolate());

                // If the value is empty, return Undefined.
                if (_maybe_script_value->IsEmpty())
                    return handle_scope.Escape(v8::Undefined(isolate));

                // Otherwise return the value.
                return handle_scope.Escape(_maybe_script_value->Get(isolate));
            }

            // No preconditions.
            void set_script_value(engine::CallContextS call_context, const v8::Local<v8::Value> &value) {
                if (value->IsUndefined()) {
                    erase();
                } else {
                    if (_maybe_script_value.has_value()) {
                        _maybe_script_value->Reset(call_context->get_isolate(), value);
                    } else {
                        _maybe_script_value.emplace(call_context->get_isolate(), value);
                    }
                }
            }
        };

        class CurrentValue {
            CellStateS _cell_state;
        public:
            CurrentValue(CellStateS cell_state) :
                    _cell_state{std::move(cell_state)} {}
            CurrentValue(const CurrentValue &) = delete;
            CurrentValue(CurrentValue &&) = delete;
            // No preconditions.
            [[nodiscard]] bool exists() const {
                return _cell_state->current_exists();
            }
            // No preconditions.
            // Returns whether the value existed previously.
            bool erase() {
                return _cell_state->erase_current();
            }
            // - Must not call when exists() == false
            [[nodiscard]] CellView get_cell() const {
                assert(exists());
                return _cell_state->get_current();
            }
            // No preconditions.
            void set_cell(Cell cell) {
                _cell_state->set_current(std::move(cell));
            }
            // Loads the script value into the current value if there were changes.
            // - ScriptValue known() must be true.
            // Returns whether changes were made.
            [[nodiscard]] engine::EngineResultCode<bool> apply(engine::CallContextS call_context,
                                                               const ScriptValue &script_value,
                                                               std::optional<TrackerS> maybe_tracker) {
                using Result = engine::EngineResultCode<bool>;
                assert(script_value.known());

                if (!script_value.exists()) {
                    return Result::Ok(erase());
                }

                V8_SCOPE(call_context->get_isolate());

                const auto value = script_value.get_script_value(call_context);

                //Serialize the Value
                fbs::Builder value_builder{};
                ValueUnionProto type;
                auto value_root_r = engine::javascript::serialize(call_context->get_log_context(), isolate,
                                                                  value_builder, value, type, maybe_tracker);
                if (!value_root_r)
                    return Result::Error(value_root_r.get_error());
                auto value_root = value_root_r.unwrap();
                value_builder.Finish(value_root);

                //Calculate the new checksum
                const auto new_checksum = get_crc(flatbuffers::GetRoot<ValueProto>(value_builder.GetBufferPointer()));

                if (exists() && get_cell()->checksum() == new_checksum) {
                    return Result::Ok(false); // checksum is still the same, no changes.
                }

                //Serialize the Cell
                fbs::Builder cell_builder{value_builder.GetSize()};
                auto value_vec_off = cell_builder.CreateVector(value_builder.GetBufferPointer(), value_builder.GetSize());
                auto cell_root = CreateCellProto(cell_builder, new_checksum, value_vec_off);

                _cell_state->set_current(finish_and_copy_to_buffer(cell_builder, call_context->get_buffer_pool(), cell_root));

                return Result::Ok(true);
            }

            // No preconditions.
            void revert() {
                _cell_state->apply_saved_to_current();
            }
        };

        engine::UnitEngineResultCode ScriptValue::apply(engine::CallContextS call_context, const CurrentValue &current) {
            using Result = engine::UnitEngineResultCode;
            if (!current.exists()) {
                erase();
            } else {
                const auto cell = current.get_cell();
                const auto *value_proto = cell->value_bytes_nested_root();
                V8_ESCAPABLE_SCOPE(call_context->get_isolate());
                UNWRAP_OR_RETURN(value, engine::javascript::deserialize(call_context->get_log_context(), call_context->get_transaction(), isolate,
                                                                        value_proto));
                if (value->IsUndefined()) {
                    erase();
                } else {
                    _maybe_script_value.emplace(isolate, handle_scope.Escape(value));
                }
            }
            return Result::Ok();
        }

        class SavedValue {
            const std::string _key;
            CellStateS _cell_state;
        public:
            SavedValue(std::string key, CellStateS cell_state) :
                    _key{std::move(key)}, _cell_state{std::move(cell_state)} {}
            SavedValue(const SavedValue &) = delete;
            SavedValue(SavedValue &&) = delete;
            // No preconditions.
            [[nodiscard]] bool exists() const {
                return _cell_state->saved_exists();
            }
            // - Must not call when exists() == false
            [[nodiscard]] CellView get_cell() const {
                assert(exists());
                return _cell_state->get_saved();
            }
            // No preconditions.
            [[nodiscard]] ResultCode<bool> apply(engine::CallContextS call_context, const CurrentValue &current) {
                using Result = ResultCode<bool>;
                if (!current.exists()) {
                    if (!exists())
                        return Result::Ok(false);
                    auto txn = call_context->get_transaction();
                    WORKED_OR_RETURN(txn->delete_value(_key));
                } else {
                    const auto current_cell = current.get_cell();
                    if (exists() && current_cell->checksum() == get_cell()->checksum())
                        return Result::Ok(false); //no changes
                    auto txn = call_context->get_transaction();
                    WORKED_OR_RETURN(txn->write_cell(current_cell, _key));
                }
                _cell_state->apply_current_to_saved();
                return Result::Ok(true);
            }
        };

        std::optional<i32> maybe_get_checksum(const std::optional<Cell> &maybe_cell) {
            return maybe_cell.has_value() ? std::make_optional<i32>(maybe_cell.value()->checksum()) : std::nullopt;
        }

        // Class that encapsulates reading/writing individual object fields (properties)
        class Property {
            engine::CallContextS _call_context;
            const std::string _name;
            const std::optional<i32> _maybe_original_checksum; //used to determine if there's any changes since the very beginning.
            CurrentValue _current_value;
            SavedValue _saved_value;
            ScriptValue _script_value{};
        public:
            Property(engine::CallContextS call_context, std::string name, std::string key,
                     std::optional<i32> maybe_original_checksum, CellStateS cell_state)
                    :
                    _call_context{std::move(call_context)},
                    _name{name},
                    _maybe_original_checksum{std::move(maybe_original_checksum)},
                    _saved_value{std::move(key), cell_state},
                    _current_value{cell_state} {}
            Property(const Property &) = delete;
            Property(Property &&) = delete;

            // Whether the current state of the property exists.
            // Note: new properties will return false until they've been flushed.
            // No preconditions.
            [[nodiscard]] bool exists() const {
                return _current_value.exists();
            }
            // Whether the saved state of the property exists.
            // No preconditions.
            [[nodiscard]] bool saved_exists() const {
                return _saved_value.exists();
            }
            // No preconditions.
            // Erases the property. Returns whether it existed previously in the current state.
            bool erase(bool script_value_only) {
                if (script_value_only) {
                    return _script_value.erase();
                } else {
                    return _script_value.erase() | _current_value.erase();
                }
            }
            // Gets the current cell.
            // - Must not call when exists() == false
            [[nodiscard]] const CellView get_cell() {
                assert(exists());
                return _current_value.get_cell();
            }
            // Gets the cell that exists in the database if it exists
            // - Must not call when saved_exists() == false
            [[nodiscard]] const CellView get_saved_cell() const {
                assert(saved_exists());
                return _saved_value.get_cell();
            }
            // Sets the cell value.
            // No preconditions.
            void set_cell(Cell cell) {
                _current_value.set_cell(std::move(cell));
            }
            // Gets the script value or Undefined if it doesn't exist.
            // No preconditions.
            [[nodiscard]] engine::EngineResultCode<v8::Local<v8::Value>> get_script_value() {
                using Result = engine::EngineResultCode<v8::Local<v8::Value>>;
                if (!_script_value.known()) {
                    WORKED_OR_RETURN(_script_value.apply(_call_context, _current_value));
                }
                return Result::Ok(_script_value.get_script_value(_call_context));
            }
            // Sets the script value.
            // No preconditions.
            void set_script_value(const v8::Local<v8::Value> &value) {
                _script_value.set_script_value(_call_context, value);
            }
            [[nodiscard]] const std::string_view get_name_view() const {
                return _name;
            }
            [[nodiscard]] const std::string &get_name() const {
                return _name;
            }
            // Flushes the script value into the current value.
            // No preconditions.
            [[nodiscard]] engine::EngineResultCode<bool> flush(std::optional<TrackerS> maybe_tracker) {
                using Result = engine::EngineResultCode<bool>;
                if (!_script_value.known()) {
                    return Result::Ok(false); //nothing to flush
                }
                return _current_value.apply(_call_context, _script_value, maybe_tracker);
            }
            // Save's the state to the database if there was a change and replaces the saved state if necessary.
            // No preconditions.
            [[nodiscard]] ResultCode<bool> save() {
                using Result = ResultCode<bool>;
                return _saved_value.apply(_call_context, _current_value);
            }
            // No preconditions.
            // Reverts the current state to saved.
            void revert() {
                _current_value.revert();
                _script_value.set_unknown();
            }
        };

        struct PropertyFactory {
            static ResultCode<PropertyS> LoadOrCreate(engine::CallContextS call_context, const ObjectReferenceS &ref, std::string name) {
                using Result = ResultCode<PropertyS>;
                auto key = create_property_key(ref->class_id, ref->get_primary_key(), name);
                UNWRAP_OR_RETURN(maybe_cell, call_context->get_transaction()->maybe_get_cell(key));

                std::optional<i32> maybe_original_checksum{};
                CellStateS cell_state;
                if (maybe_cell.has_value()) {
                    //Existing
                    maybe_original_checksum = std::make_optional<i32>(maybe_cell.value()->checksum());
                    cell_state = std::make_shared<CellState>(std::move(maybe_cell.value()));
                } else {
                    //New (note: exists() will return false initially)
                    cell_state = std::make_shared<CellState>();
                }

                return Result::Ok(std::make_shared<Property>(std::move(call_context), std::move(name),
                                                             std::move(key), maybe_original_checksum, std::move(cell_state)));
            }
        };

        enum class DeleteOption {
            NOT_DELETED = 0,
            DELETE = 1,
            PURGE = 2
        };

        class CurrentObjectState {
            engine::CallContextS _call_context;
            DeleteOption _delete_option{DeleteOption::NOT_DELETED};
            std::set<std::string> _property_names;
            std::unordered_map<std::string_view, PropertyS> _property_cache{};
        public:
            CurrentObjectState(engine::CallContextS call_context, std::set<std::string> property_names)
                    :
                    _call_context{std::move(call_context)},
                    _property_names{std::move(property_names)} {}
            CurrentObjectState(const CurrentObjectState &) = delete;
            CurrentObjectState(CurrentObjectState &&) = delete;
        public:
            engine::CallContextS get_call_context() const {
                return _call_context;
            }

            // No preconditions.
            // Returns whether a change was made.
            bool set_is_deleted(bool purge) {
                const auto prev = _delete_option;
                _delete_option = purge ? DeleteOption::PURGE : DeleteOption::DELETE;
                return prev != _delete_option;
            }

            // No preconditions.
            [[nodiscard]] bool is_deleted() const {
                return _delete_option != DeleteOption::NOT_DELETED;
            }

            // No preconditions.
            [[nodiscard]] bool is_purged() const {
                return _delete_option == DeleteOption::PURGE;
            }

            // No preconditions.
            const std::set<std::string> &get_property_names() const {
                return _property_names;
            }

            // No preconditions.
            [[nodiscard]] ResultCode<PropertyS> get_property(const ObjectReferenceS &ref, std::string name) {
                using Result = ResultCode<PropertyS>;

                const auto it = _property_cache.find(name);
                if (it != _property_cache.end())
                    return Result::Ok(it->second); //Found, return cached

                UNWRAP_OR_RETURN(property, PropertyFactory::LoadOrCreate(_call_context, ref, std::move(name)));
                _property_names.emplace(property->get_name());
                _property_cache[property->get_name_view()] = property;
                return Result::Ok(std::move(property));
            }

            // No preconditions.
            [[nodiscard]] const std::unordered_map<std::string_view, PropertyS> &get_property_cache() const {
                return _property_cache;
            }

            // No preconditions.
            [[nodiscard]] bool erase_property(PropertyS &property, bool script_value_only) {
                _property_names.erase(property->get_name());
                return property->erase(script_value_only);
            }

            // No preconditions.
            [[nodiscard]] engine::EngineResultCode<bool> flush(std::optional<TrackerS> maybe_tracker) {
                using Result = engine::EngineResultCode<bool>;
                bool any_changed{false};
                for (auto[name, property]: _property_cache) {
                    UNWRAP_OR_RETURN(changed, property->flush(maybe_tracker));
                    if (changed) {
                        any_changed = true;
                    }
                }
                return Result::Ok(any_changed);
            }

            // No preconditions.
            void revert(const PermanentObjectState &permanent);
        };

        class PermanentObjectState {
            bool _is_deleted{false};
            std::set<std::string> _property_names;
            std::unordered_map<std::string_view, PropertyS> _changed_properties{};
        public:
            PermanentObjectState(std::set<std::string> property_names) :
                    _property_names{std::move(property_names)} {}
            PermanentObjectState(const PermanentObjectState &) = delete;
            PermanentObjectState(PermanentObjectState &&) = delete;

            // No preconditions.
            constexpr bool is_deleted() const {
                return _is_deleted;
            }

            // No preconditions.
            // All the property names that exist.
            const std::set<std::string> &get_property_names() const {
                return _property_names;
            }

            // No preconditions.
            // Gets all the properties that have had changes.
            const std::unordered_map<std::string_view, PropertyS> &get_changed_properties() const {
                return _changed_properties;
            }

            // No preconditions.
            // Applies the current state to the saved state.
            // Returns true if changes were made, false otherwise.
            ResultCode<bool> apply(ObjectHandleS handle, CurrentObjectState &current);
        };

        ResultCode<bool> PermanentObjectState::apply(ObjectHandleS handle, CurrentObjectState &current) {
            using Result = ResultCode<bool>;

            bool any_changes{false};

            //Saves the whole object ot the database.

            if (current.is_deleted()) {
                if (!is_deleted() || current.is_purged()) {
                    auto txn = current.get_call_context()->get_transaction();

                    //delete all the properties that the property index points to
                    const auto ref = handle->get_reference();
                    for (const auto &property_name: current.get_property_names()) {
                        auto property_key = create_property_key(ref->class_id, ref->get_primary_key(), property_name);
                        WORKED_OR_RETURN(txn->delete_value(property_key));
                    }

                    //delete the property index
                    WORKED_OR_RETURN(txn->delete_value(ref->get_object_properties_index_key()));

                    handle->increment_version();

                    if (current.is_purged()) {
                        //delete the object instance so another object can be recreated with the same PK.
                        WORKED_OR_RETURN(txn->delete_value(ref->get_object_instance_key()));
                    } else {
                        //mark the object as deleted so another can't be created in its place.
                        WORKED_OR_RETURN(txn->write_object_instance(handle->get_reference(), handle->get_version(), true));
                    }

                    any_changes = true;
                    _is_deleted = true;
                }
            } else {
                // Save all the properties in the cache
                for (auto[name, property]: current.get_property_cache()) {
                    UNWRAP_OR_RETURN(changed, property->save());
                    if (changed) {
                        _changed_properties[property->get_name_view()] = property;
                        any_changes = true;
                    }
                }

                auto txn = current.get_call_context()->get_transaction();

                //Update the properties index if the property names have changed
                if (current.get_property_names() != _property_names) {
                    WORKED_OR_RETURN(txn->write_object_properties_index(handle->get_reference(), current.get_property_names()));
                    any_changes = true;
                    _property_names = current.get_property_names();
                }

                if (any_changes) {
                    handle->increment_version();
                    WORKED_OR_RETURN(txn->write_object_instance(handle->get_reference(), handle->get_version(), false));
                }
            }

            return Result::Ok(any_changes);
        }

        void CurrentObjectState::revert(const PermanentObjectState &permanent) {
            _delete_option = permanent.is_deleted() ? DeleteOption::DELETE : DeleteOption::NOT_DELETED;
            _property_names = permanent.get_property_names();
            for (auto[_, property]: _property_cache) {
                property->revert();
            }
        }

        class ObjectFactory {
            [[nodiscard]] static ResultCode<std::optional<ObjectS>> MaybeLoad(engine::CallContextS call_context, ObjectReferenceS ref,
                                                                              std::variant<bool, ObjectVersion> create_if_not_found_or_expected_version) {
                using Result = ResultCode<std::optional<ObjectS>>;

                // Mutually exclusive options
                bool create_if_not_found{false};
                std::optional<ObjectVersion> maybe_expected_version{};
                if (std::holds_alternative<bool>(create_if_not_found_or_expected_version)) {
                    create_if_not_found = std::get<bool>(create_if_not_found_or_expected_version);
                } else {
                    maybe_expected_version = std::get<ObjectVersion>(create_if_not_found_or_expected_version);
                }

                auto txn = call_context->get_transaction();

                // Get the ObjectInstanceProto which contains the very basics.
                UNWRAP_OR_RETURN(maybe_object_instance, txn->maybe_get_object_instance(ref));
                if (!maybe_object_instance.has_value()) {
                    if (create_if_not_found) {
                        auto object_r = CreateNew(call_context, ref);
                        if (!object_r)
                            return Result::Error(object_r.get_error());
                        return Result::Ok(object_r.unwrap());
                    }
                    return Result::Ok(std::nullopt);
                }

                auto object_instance = std::move(maybe_object_instance.value());

                if ((data::ObjectType) object_instance->type() != ref->type)
                    return Result::Error(Code::Datastore_TypeMismatch);

                if (object_instance->deleted())
                    return Result::Error(Code::Datastore_ObjectDeleted);

                if (maybe_expected_version.has_value()) {
                    if (object_instance->version() > maybe_expected_version.value()) {
                        // User is out of date.
                        return Result::Error(Code::Datastore_MustGetLatestObject); //user has the wrong version
                    }
                    if (object_instance->version() < maybe_expected_version.value()) {
                        // Somehow they got the wrong version. Maybe they're using a custom client?
                        return Result::Error(Code::Datastore_ClientHasVersionInvalid);
                    }
                }

                // Get the property names if they exist.
                UNWRAP_OR_RETURN(maybe_object_index_properties, txn->maybe_get_object_properties_index(ref));
                std::optional<std::set<std::string>> maybe_property_names{};
                if (maybe_object_index_properties.has_value()) {
                    const auto object_index_properties = std::move(maybe_object_index_properties.value());
                    if (object_index_properties->properties() && object_index_properties->properties()->size() > 0) {
                        maybe_property_names.emplace();
                        for (auto it: *object_index_properties->properties()) {
                            maybe_property_names->insert(it->str());
                        }
                    }
                }

                const auto version = object_instance->version();
                auto handle = make_object_handle(std::move(ref), version, version);

                if (maybe_property_names.has_value()) {
                    return Result::Ok(std::make_shared<Object>(std::move(call_context), std::move(handle), std::move(maybe_property_names.value())));
                } else {
                    return Result::Ok(std::make_shared<Object>(std::move(call_context), std::move(handle), std::set<std::string>{}));
                }
            }
        public:
            [[nodiscard]] static ResultCode<std::optional<ObjectS>>
            MaybeLoadFromRef(engine::CallContextS call_context, ObjectReferenceS ref, bool create_if_not_found) {
                return MaybeLoad(std::move(call_context), std::move(ref), create_if_not_found);
            }
            [[nodiscard]] static ResultCode<std::optional<ObjectS>> MaybeLoadFromHandle(engine::CallContextS call_context, ObjectHandleS handle) {
                return MaybeLoad(std::move(call_context), handle->get_reference(), handle->get_version());
            }
            [[nodiscard]] static ResultCode<ObjectS> CreateNew(engine::CallContextS call_context, ObjectReferenceS ref) {
                using Result = ResultCode<ObjectS>;
                auto txn = call_context->get_transaction();
                UNWRAP_OR_RETURN(exists, txn->object_instance_exists(ref));
                if (exists)
                    return Result::Error(Code::Datastore_DuplicateObject);
                auto handle = make_object_handle(std::move(ref), 0, 0);
                return Result::Ok(std::make_shared<Object>(std::move(call_context), std::move(handle), std::set<std::string>{}));
            }
        };

        class Object {
            engine::CallContextS _call_context;
            const ObjectHandleS _handle;
            PermanentObjectState _permanent;
            CurrentObjectState _current;
            std::optional<ObjectVersion> _delta_version{};
        public:
            Object(engine::CallContextS call_context, ObjectHandleS handle, std::set<std::string> property_names) :
                    _call_context{call_context}, _handle{std::move(handle)}, _permanent{property_names},
                    _current{std::move(call_context), property_names} {}
            Object(Object &&) = delete;
            Object(const Object &) = delete;
        public:
            [[nodiscard]] bool needs_delta() const {
                if (_handle->get_original_version() != _handle->get_version()) {
                    if (!_delta_version.has_value()) {
                        return true;
                    }
                    return _delta_version.value() < _handle->get_version();
                }
                return false;
            }
            void update_delta_version() {
                _delta_version = _handle->get_version();
            }
            // No preconditions.
            [[nodiscard]] bool is_deleted() const {
                return _current.is_deleted();
            }
            // No preconditions.
            bool set_is_deleted(bool purge) {
                return _current.set_is_deleted(purge);
            }
            // No preconditions.
            [[nodiscard]] bool has_saved() const {
                return _handle->get_version() != _handle->get_original_version();
            }
            // No preconditions.
            [[nodiscard]] bool is_new() const {
                return _handle->get_version() == 0;
            }
            // No preconditions.
            [[nodiscard]] bool is_permanent_deleted() const {
                return _permanent.is_deleted();
            }
            // No preconditions.
            [[nodiscard]] const bool is_data() const {
                return _handle->is_data();
            }
            // No preconditions.
            [[nodiscard]] const bool is_service() const {
                return _handle->is_service();
            }
            // No preconditions.
            [[nodiscard]] const ObjectReferenceS get_reference() const {
                return _handle->get_reference();
            }
            // No preconditions.
            [[nodiscard]] const ObjectHandleS get_handle() const {
                return _handle;
            }
            // No preconditions.
            [[nodiscard]] const std::set<std::string> &get_property_names() const {
                return _current.get_property_names();
            }
            // No preconditions.
            [[nodiscard]] const std::set<std::string> &get_permanent_property_names() const {
                return _permanent.get_property_names();
            }
            // No preconditions.
            [[nodiscard]] ResultCode<PropertyS> get_property(std::string property_name) {
                return _current.get_property(get_reference(), property_name);
            }
            // No preconditions.
            bool erase_property(PropertyS &property, bool script_value_only) {
                return _current.erase_property(property, script_value_only);
            }
            // No preconditions.
            ObjectVersion get_version() const {
                return _handle->get_version();
            }
            // No preconditions
            [[nodiscard]] const std::unordered_map<std::string_view, PropertyS> &get_property_cache() const {
                return _current.get_property_cache();
            }
            // No preconditions.
            [[nodiscard]] const std::unordered_map<std::string_view, PropertyS> &get_permanent_property_changes() const {
                return _permanent.get_changed_properties();
            }
            // No preconditions.
            // Flushes the script values to the current state
            // Returns true if any changes were made, false otherwise.
            [[nodiscard]] engine::EngineResultCode<bool> flush(std::optional<TrackerS> maybe_tracker) {
                return _current.flush(maybe_tracker);
            }
            // No preconditions.
            // Writes the current state to the database and updates the permanent state.
            // Returns true if there were any changes, false otherwise.
            [[nodiscard]] ResultCode<bool> save() {
                using Result = ResultCode<bool>;
                return _permanent.apply(_handle, _current);
            }
            // No preconditions.
            // Flushes the script values to the current state, writes the current state to the database and overwrites the permanent state.
            // Returns true if there were any changes, false otherwise.
            [[nodiscard]] engine::EngineResultCode<bool> flush_and_save(std::optional<TrackerS> maybe_tracker) {
                using Result = engine::EngineResultCode<bool>;
                UNWRAP_OR_RETURN(flush_changes, flush(maybe_tracker));
                UNWRAP_OR_RETURN(save_changes, save());
                return Result::Ok(flush_changes || save_changes);
            }
            // No preconditions.
            // Attempts to flush and save the object and returns true if any changes were found.
            [[nodiscard]] engine::EngineResultCode<bool> was_already_saved() {
                using Result = engine::EngineResultCode<bool>;
                UNWRAP_OR_RETURN(flushed, flush(std::nullopt));
                if (flushed)
                    return Result::Ok(false);
                UNWRAP_OR_RETURN(saved, save());
                if (saved)
                    return Result::Ok(false);
                return Result::Ok(true);
            }
            // No preconditions.
            // Reverts the current state back to its permanent state.
            void revert() {
                _current.revert(_permanent);
            }
        };

        class WorkingSet {
            const bool _cached;
            engine::CallContextS _call_context;
            std::optional<std::unordered_map<ObjectReferenceWrapper, ObjectHandleS, ObjectReferenceWrapper::Hasher>> _maybe_handles{};
            std::optional<std::unordered_map<ObjectHandleWrapper, ObjectS, ObjectHandleWrapper::Hasher>> _maybe_objects{};
        public:
            [[nodiscard]] std::optional<ObjectHandleS> maybe_resolve_reference(const ObjectReferenceS &ref) {
                assert(_cached);
                if (!_maybe_handles.has_value())
                    return std::nullopt;
                const auto it = _maybe_handles->find(ObjectReferenceWrapper{ref});
                if (it != _maybe_handles->end())
                    return it->second;
                return std::nullopt;
            }
            [[nodiscard]] std::optional<ObjectS> maybe_resolve_handle(const ObjectHandleS &handle) {
                assert(_cached);
                if (!_maybe_objects.has_value())
                    return std::nullopt;
                const auto it = _maybe_objects->find(ObjectHandleWrapper{handle});
                if (it != _maybe_objects->end())
                    return it->second;
                return std::nullopt;
            }
            void cache(ObjectS object) {
                assert(_cached);
                auto handle = object->get_handle();

                if (!_maybe_handles.has_value()) {
                    assert(!_maybe_objects.has_value());
                    _maybe_handles.emplace();
                    _maybe_objects.emplace();
                }

                ObjectReferenceWrapper ref_wrapper{handle->get_reference()};
                ObjectHandleWrapper handle_wrapper{handle};

                _maybe_handles->insert(std::make_pair(std::move(ref_wrapper), handle));
                _maybe_objects->insert(std::make_pair(std::move(handle_wrapper), std::move(object)));
            }
        public:
            WorkingSet(bool cached, engine::CallContextS call_context) :
                    _cached{cached}, _call_context{std::move(call_context)} {}
            WorkingSet(const WorkingSet &) = delete;
            WorkingSet(WorkingSet &&) = delete;
            ~WorkingSet() = default;
            [[nodiscard]] ResultCode<ObjectS> resolve(ObjectReferenceS ref, bool create_if_not_found) {
                using Result = ResultCode<ObjectS>;
                UNWRAP_OR_RETURN(maybe_object, maybe_resolve(std::move(ref), create_if_not_found));
                if (!maybe_object.has_value())
                    return Result::Error(Code::Datastore_ObjectNotFound);
                return Result::Ok(std::move(maybe_object.value()));
            }
            [[nodiscard]] ResultCode<ObjectS> resolve(ObjectHandleS handle) {
                using Result = ResultCode<ObjectS>;
                UNWRAP_OR_RETURN(maybe_object, maybe_resolve(std::move(handle)));
                if (!maybe_object.has_value())
                    return Result::Error(Code::Datastore_ObjectNotFound);
                return Result::Ok(std::move(maybe_object.value()));
            }
            [[nodiscard]] ResultCode<ObjectS> create(ObjectReferenceS ref) {
                using Result = ResultCode<ObjectS>;
                if (maybe_resolve_reference(ref)) //early-detect it's a duplicate
                    return Result::Error(Code::Datastore_DuplicateObject);
                UNWRAP_OR_RETURN(object, ObjectFactory::CreateNew(_call_context, ref));
                if (_cached)
                    cache(object);
                return Result::Ok(std::move(object));
            }
            [[nodiscard]]  ResultCode<std::optional<ObjectS>> maybe_resolve(ObjectReferenceS ref, bool create_if_not_found) {
                using Result = ResultCode<std::optional<ObjectS>>;

                if (_cached) {
                    //Try to resolve from cache
                    auto maybe_handle = maybe_resolve_reference(ref);
                    if (maybe_handle) {
                        auto maybe_object = maybe_resolve_handle(maybe_handle.value());
                        if (maybe_object) {
                            return Result::Ok(std::move(maybe_object.value()));
                        }
                    }
                }

                UNWRAP_OR_RETURN(maybe_object, ObjectFactory::MaybeLoadFromRef(_call_context, ref, create_if_not_found));

                if (!maybe_object.has_value()) {
                    return Result::Ok(std::nullopt);
                } else {
                    if (_cached)
                        cache(maybe_object.value());
                    return Result::Ok(std::move(maybe_object));
                }
            }
            [[nodiscard]]  ResultCode<std::optional<ObjectS>> maybe_resolve(ObjectHandleS handle) {
                using Result = ResultCode<std::optional<ObjectS>>;

                if (_cached) { //Try to resolve from cache
                    auto maybe_object = maybe_resolve_handle(handle);
                    if (maybe_object) {
                        return Result::Ok(std::move(maybe_object));
                    }
                }

                UNWRAP_OR_RETURN(maybe_object, ObjectFactory::MaybeLoadFromHandle(_call_context, handle));

                if (!maybe_object.has_value()) {
                    return Result::Ok(std::nullopt);
                } else {
                    if (_cached)
                        cache(maybe_object.value());
                    return Result::Ok(std::move(maybe_object));
                }
            }
            [[nodiscard]] const std::unordered_map<ObjectHandleWrapper, ObjectS, ObjectHandleWrapper::Hasher> &get_cached_objects() const {
                assert(_cached);
                return _maybe_objects.value();
            }
        };

        const PrimaryKey &ObjectReference::get_primary_key() const {
            assert(!_moved);
            return _primary_key;
        }
        std::string_view ObjectReference::get_object_properties_index_key() {
            assert(!_moved);
            if (!_object_properties_index_key.has_value()) {
                _object_properties_index_key = create_object_properties_index_key(class_id, _primary_key);
            }
            return std::string_view{_object_properties_index_key.value().data(), _object_properties_index_key.value().size()};
        }
        std::string_view ObjectReference::get_object_instance_key() {
            assert(!_moved);
            if (!_object_instance_key.has_value()) {
                _object_instance_key = create_object_instance_key(class_id, _primary_key);
            }
            return std::string_view{_object_instance_key.value().data(), _object_instance_key.value().size()};
        }
        ObjectReference::ObjectReference(ObjectType type, ClassId class_id, PrimaryKey primary_key) :
                type(type), class_id(class_id), _primary_key(std::move(primary_key)), _moved(false) {}
        ObjectReference::ObjectReference(ObjectReference &&other) noexcept:
                _object_instance_key(std::move(other._object_instance_key)),
                _object_properties_index_key(std::move(other._object_properties_index_key)),
                type(other.type), _primary_key(std::move(other._primary_key)), class_id(other.class_id), _moved(false) {
            other._moved = true;
        }
        bool ObjectReference::is_data() const { return this->type == ObjectType::WORKER_OBJECT; }
        bool ObjectReference::is_service() const { return this->type == ObjectType::WORKER_SERVICE; }
        bool ObjectReference::operator==(const ObjectReference &rhs) const {
            return _primary_key == rhs._primary_key &&
                   type == rhs.type &&
                   class_id == rhs.class_id;
        }
        bool ObjectReference::operator!=(const ObjectReference &rhs) const {
            return !(rhs == *this);
        }
        ObjectReferenceS make_object_reference(ObjectType type, ClassId class_id, PrimaryKey primary_key) {
            return std::make_shared<ObjectReference>(type, class_id, std::move(primary_key));
        }
        ObjectHandle::ObjectHandle(ObjectReferenceS object_reference, ObjectVersion original_version, ObjectVersion version)
                :
                _reference{std::move(object_reference)}, _original_version{original_version}, _version{version} {
        }
        ObjectReferenceS ObjectHandle::get_reference() const {
            return _reference;
        }
        ObjectVersion ObjectHandle::get_original_version() const {
            return _original_version;
        }
        ObjectVersion ObjectHandle::get_version() const {
            return _version;
        }
        void ObjectHandle::increment_version() {
            _version = _version + 1;
        }
        bool ObjectHandle::is_data() const {
            return _reference->is_data();
        }
        bool ObjectHandle::is_service() const {
            return _reference->is_service();
        }
        bool ObjectHandle::operator==(const ObjectHandle &rhs) const {
            return *this->_reference == *rhs._reference && this->_original_version == rhs._original_version;
        }
        bool ObjectHandle::operator!=(const ObjectHandle &rhs) const {
            return !(rhs == *this);
        }
        ObjectHandleS make_object_handle(ObjectReferenceS object_reference, ObjectVersion original_version, ObjectVersion version) {
            return std::make_shared<ObjectHandle>(std::move(object_reference), original_version, version);
        }

        ResultCode<fbs::Offset<fbs::Vector<fbs::Offset<NestedDataProto>>>> export_data(
                fbs::Builder &target_builder, const data::ObjectReferenceS &start, WorkingSetS working_set) {
            using Result = ResultCode<fbs::Offset<fbs::Vector<fbs::Offset<NestedDataProto>>>>;
            std::unordered_set<data::ObjectReferenceWrapper, data::ObjectReferenceWrapper::Hasher> found{};
            std::queue<data::ObjectReferenceS> to_write{};

            to_write.push(start);

            std::vector<fbs::Offset<NestedDataProto>> data_protos{};
            while (!to_write.empty()) {
                auto ref = to_write.front();
                to_write.pop();

                UNWRAP_OR_RETURN(maybe_object, working_set->maybe_resolve(ref, false));
                if (!maybe_object) {
                    if (*ref == *start) {
                        //if the start object is missing, then return object not found.
                        return Result::Error(Code::Datastore_ObjectNotFound);
                    }
                    //ignore all the other missing objects
                    continue;
                }
                auto object = std::move(maybe_object.value());

                if (object->is_permanent_deleted()) {
                    if (*ref == *start) {
                        //if the start object is deleted, then return object deleted.
                        return Result::Error(Code::Datastore_ObjectDeleted);
                    }
                    continue; //just leave the dangling reference so the client can issue a warning that the target object no longer exists.
                }

                std::vector<fbs::Offset<NestedPropertyProto>> properties{};

                for (const auto &property_name: object->get_permanent_property_names()) {
                    UNWRAP_OR_RETURN(property, object->get_property(property_name));
                    if (!property->saved_exists())
                        continue;

                    const auto cell = property->get_saved_cell();

                    auto name_off = target_builder.CreateString(property_name);
                    const auto value = cell->value_bytes_nested_root();

                    //Save the reference for later resolution if it hasn't already been found.
                    if (value->value_type() == ValueUnionProto::DataReferenceValueProto) {
                        const auto ref_value = value->value_as_DataReferenceValueProto();
                        const auto prop_ref = make_object_reference(data::ObjectType::WORKER_OBJECT, ref_value->class_id(),
                                                                    PrimaryKey{ref_value->primary_key()->str()});
                        ObjectReferenceWrapper ref_wrapper{prop_ref};
                        if (!found.contains(ref_wrapper)) {
                            to_write.push(std::move(prop_ref));
                            found.insert(std::move(ref_wrapper));
                        }
                    }

                    //Copy the value directly (does a memcpy internally)
                    const auto value_bytes = cell->value_bytes();
                    auto value_vec_off = target_builder.CreateVector(value_bytes->data(), value_bytes->size());
                    properties.push_back(CreateNestedPropertyProto(target_builder, name_off, value_vec_off));
                }

                const auto primary_key_off = target_builder.CreateString(ref->get_primary_key().view());
                data_protos.push_back(CreateNestedDataProto(target_builder, ref->class_id, object->get_version(), primary_key_off,
                                                                         false, target_builder.CreateVector(properties)));
            }

            return Result::Ok(target_builder.CreateVector(data_protos));
        }
        std::vector<Buffer<DataDeltaProto>>
        export_deltas_from_working_set(const LogContext &log_context, BufferPoolS buffer_pool, fbs::BuilderS reusabled_builder,
                                       WorkingSetS working_set) {
            std::vector<data::ObjectS> objects{};
            for (auto[_, object]: working_set->get_cached_objects()) {
                if (object->has_saved())
                    objects.push_back(object);
            }
            std::vector<Buffer<DataDeltaProto>> deltas{};
            if (objects.size() > 0) {
                auto result = create_deltas(log_context, buffer_pool, reusabled_builder, objects);
                for (auto&[_, delta]: result) {
                    deltas.emplace_back(std::move(delta));
                }
            }
            return std::move(deltas);
        }
        std::vector<std::pair<ObjectHandleS, Buffer<DataDeltaProto>>> create_deltas(const LogContext &log_context,
                                                                                          BufferPoolS buffer_pool,
                                                                                          fbs::BuilderS reusable_builder,
                                                                                          std::vector<data::ObjectS> &objects) {
            std::vector<std::pair<ObjectHandleS, Buffer<DataDeltaProto>>> deltas{};
            for (const auto object: objects) {
                if (!object->needs_delta())
                    continue;
                reusable_builder->Reset();
                if (object->is_permanent_deleted()) {
                    deltas.emplace_back(std::make_pair(
                            object->get_handle(),
                            finish_and_copy_to_buffer(*reusable_builder, buffer_pool,
                                                      CreateDataDeltaProto(*reusable_builder,
                                                                                 object->get_reference()->class_id,
                                                                                 object->get_handle()->get_version(),
                                                                                 reusable_builder->CreateString(
                                                                                         object->get_reference()->get_primary_key().view()),
                                                                                 true,
                                                                                 0,
                                                                                 0
                                                      ))));
                    object->update_delta_version();
                } else {
                    std::vector<fbs::Offset<fbs::String>> deleted{};
                    std::vector<fbs::Offset<NestedPropertyProto>> updated{};
                    for (const auto&[name, property]: object->get_permanent_property_changes()) {
                        if (!property->saved_exists()) {
                            deleted.push_back(reusable_builder->CreateString(name));
                            continue;
                        } else {
                            auto name_off = reusable_builder->CreateString(name);
                            const auto value_bytes = property->get_saved_cell()->value_bytes();
                            const auto vec_off = reusable_builder->CreateVector(value_bytes->data(), value_bytes->size());
                            updated.push_back(CreateNestedPropertyProto(*reusable_builder, name_off, vec_off));
                        }
                    }
                    if (!updated.empty() || !deleted.empty()) {
                        deltas.emplace_back(std::make_pair(
                                object->get_handle(),
                                finish_and_copy_to_buffer(
                                        *reusable_builder, buffer_pool,
                                        CreateDataDeltaProto(*reusable_builder,
                                                                   object->get_reference()->class_id,
                                                                   object->get_handle()->get_version(),
                                                                   reusable_builder->CreateString(object->get_reference()->get_primary_key().view()),
                                                                   false,
                                                                   updated.empty() ? 0 : reusable_builder->CreateVector(updated),
                                                                   deleted.empty() ? 0 : reusable_builder->CreateVector(deleted)
                                        ))));
                        object->update_delta_version();
                    }
                }
                //sj note: this will skip objects that just have property name changes... I don't think this is possible.
            }
            return std::move(deltas);
        }
        ResultCode<std::optional<data::ObjectHandleS>>
        apply_inbound_delta(fbs::Builder &reusable_builder, const InboundDataDeltaProto &delta, WorkingSetS working_set,
                            BufferPoolS buffer_pool, bool save) {
            using Result = ResultCode<std::optional<data::ObjectHandleS>>;

            const auto ref = make_object_reference(data::ObjectType::WORKER_OBJECT, delta.class_id(),
                                                   PrimaryKey{delta.primary_key()->str()});

            const auto object_version = delta.object_version();
            data::ObjectS object;
            bool any_changes{false};
            if (object_version == 0) {
                //The delta represents a new object to be created.
                ASSIGN_OR_RETURN(object, working_set->create(ref));
                any_changes = true;
            } else {
                //The delta is a modification to an existing object.
                ASSIGN_OR_RETURN(object, working_set->resolve(make_object_handle(ref, object_version, object_version)));
            }

            // Create or Update properties
            if (delta.properties() && delta.properties()->size() > 0) {
                for (auto i = 0; i < delta.properties()->size(); ++i) {
                    auto updated_property = delta.properties()->Get(i);

                    auto property_name = updated_property->name()->str();

                    UNWRAP_OR_RETURN(property, object->get_property(updated_property->name()->str()));

                    const auto *value_bytes = updated_property->value_bytes();
                    const auto value_checksum = data::get_crc(updated_property->value_bytes_nested_root());

                    //new or updated
                    if (!property->saved_exists() || property->get_saved_cell()->checksum() != value_checksum) {
                        reusable_builder.Clear();
                        auto updated_cell = finish_and_copy_to_buffer(reusable_builder, buffer_pool,
                                                                      CreateCellProto(reusable_builder, value_checksum,
                                                                                      reusable_builder.CreateVector(value_bytes->data(),
                                                                                                                    value_bytes->size())));
                        property->set_cell(std::move(updated_cell));
                        assert(property->exists());
                        any_changes = true;
                    }
                }
            }

            // Delete properties
            if (delta.deleted_properties() && delta.deleted_properties()->size() > 0) {
                for (auto i = 0; i < delta.deleted_properties()->size(); ++i) {
                    UNWRAP_OR_RETURN(property, object->get_property(delta.deleted_properties()->Get(i)->str()));
                    if (object->erase_property(property, false))
                        any_changes = true;
                }
            }

            if (save && any_changes) {
                UNWRAP_OR_RETURN(changed, object->save());
                if (changed)
                    return Result::Ok(object->get_handle());
            }

            return Result::Ok(std::nullopt); //no changes
        }
    }
    namespace storage {
        using Status = rocksdb::Status;

        inline std::string get_worker_log_context(const WorkerId worker_id) {
            return fmt::format("(Worker {0})", worker_id);
        }

        inline std::string get_worker_version_log_context(const WorkerId worker_id, const WorkerVersion worker_version) {
            return fmt::format("(Worker {0}@{1})", worker_id, worker_version);
        }

        inline std::string get_class_log_context(const WorkerId worker_id, const ClassId class_id, const PrimaryKey &primary_key) {
            return fmt::format("(Worker {0} Class {1} PK {2})", worker_id, class_id, primary_key.view());
        }

        inline void
        log_worker_error_status(const LogContext &log_context, const WorkerId worker_id, const rocksdb::Status &s, const std::string &was_doing) {
            log_error(log_context, "{0} Database error while {1}: Code {2}, Message {3}",
                      get_worker_log_context(worker_id), was_doing, s.code(), s.ToString());
        }

        inline void
        log_worker_version_error_status(const LogContext &log_context, const WorkerId worker_id, const WorkerVersion worker_version, const rocksdb::Status &s,
                                      const std::string &was_doing) {
            log_error(log_context, "{0} Database error while {1}: Code {2}, Message {3}",
                      get_worker_version_log_context(worker_id, worker_version), was_doing, s.code(), s.ToString());
        }

        inline void
        log_object_error_status(const LogContext &log_context, const WorkerId worker_id, const ClassId class_id, const PrimaryKey &primary_key,
                                const rocksdb::Status &s,
                                const std::string &was_doing) {
            log_error(log_context, "{0} Database error while {1}: Code {2}, Message {3}",
                      get_class_log_context(worker_id, class_id, primary_key), was_doing, s.code(), s.ToString());
        }

        class DatabaseImpl;

        class TransactionImpl : public virtual ITransaction {
            const WorkerId _worker_id;
            WorkerVersion _worker_version;
            rocksdb::Transaction *_txn;
            rocksdb::ColumnFamilyHandle *_default_column_family;
            BufferPoolS _buffer_pool;
            const LogContext &_log_context;
            friend class DatabaseImpl;
            static const rocksdb::ReadOptions READ_OPTIONS;
            static const rocksdb::WriteOptions WRITE_OPTIONS;
        public:
            TransactionImpl(const TransactionImpl &other) = delete;
            TransactionImpl(TransactionImpl &&other) = delete;
            explicit TransactionImpl(const LogContext &log_context, rocksdb::Transaction *txn, rocksdb::ColumnFamilyHandle *default_column_family,
                                     const WorkerId worker_id, const WorkerVersion worker_version, BufferPoolS buffer_pool) :
                    _log_context(log_context), _txn(txn), _default_column_family(default_column_family), _worker_id(worker_id),
                    _worker_version(worker_version), _buffer_pool(buffer_pool) {
            }
            ~TransactionImpl() override {
                delete _txn;
            }
        private:
            rocksdb::Status get_for_read(const rocksdb::Slice &key, std::string &buffer) {
                return _txn->Get(READ_OPTIONS, _default_column_family, key, &buffer);
            }
            rocksdb::Status get_for_update(const rocksdb::Slice &key, std::string &buffer) {
                return _txn->GetForUpdate(READ_OPTIONS, _default_column_family, key, &buffer);
            }
        public:
            UnitResultCode write_object_instance(const data::ObjectReferenceS &ref, ObjectVersion version, bool deleted) override {
                using Result = UnitResultCode;

                flatbuffers::FlatBufferBuilder builder{ESTATE_OBJECT_INSTANCE_INITIAL_BUFFER_SIZE};

                builder.Finish(CreateObjectInstanceProto(builder, version, deleted, (uint8_t) ref->type));
                rocksdb::Slice vval{reinterpret_cast<const char *>(builder.GetBufferPointer()), builder.GetSize()};

                const auto put_s = this->_txn->Put(_default_column_family, ref->get_object_instance_key(), vval);
                if (!put_s.ok()) {
                    log_object_error_status(_log_context, _worker_id, ref->class_id, ref->get_primary_key(), put_s, "putting object instance");
                    return Result::Error(Code::Datastore_Unknown);
                }

                return Result::Ok();
            }
            UnitResultCode
            write_object_properties_index(const data::ObjectReferenceS &ref, const std::optional<std::set<std::string>> &property_names) override {
                using Result = UnitResultCode;

                flatbuffers::FlatBufferBuilder builder{ESTATE_OBJECT_PROPERTIES_INDEX_INITIAL_BUFFER_SIZE};

                if (!property_names.has_value() || property_names->empty()) {
                    builder.Reset();
                    builder.Finish(CreateObjectPropertiesIndexProto(builder, 0));
                } else {
                    builder.Reset();
                    fbs::Offset<fbs::Vector<fbs::Offset<fbs::String>>> properties_vec{};
                    std::vector<fbs::Offset<fbs::String>> properties_off_vec{};
                    for (const auto &name: property_names.value()) {
                        properties_off_vec.push_back(builder.CreateString(name));
                    }
                    properties_vec = builder.CreateVector(properties_off_vec);
                    builder.Finish(CreateObjectPropertiesIndexProto(builder, properties_vec));
                }

                rocksdb::Slice vval{reinterpret_cast<const char *>(builder.GetBufferPointer()), builder.GetSize()};
                const auto put_s = this->_txn->Put(_default_column_family, ref->get_object_properties_index_key(), vval);
                if (!put_s.ok()) {
                    log_object_error_status(_log_context, _worker_id, ref->class_id, ref->get_primary_key(), put_s, "putting object properties index");
                    return Result::Error(Code::Datastore_Unknown);
                }
                return Result::Ok();
            }
            ResultCode<bool> object_instance_exists(const data::ObjectReferenceS &ref) override {
                using Result = ResultCode<bool>;

                auto object_instance = _buffer_pool->get_buffer<ObjectInstanceProto>();
                Status status = object_instance.with_internal_buffer<Status>([&](InternalBuffer &internal_buffer) {
                    return get_for_read(ref->get_object_instance_key(), internal_buffer);
                });
                if (!status.ok() && !status.IsNotFound()) {
                    //unknown error
                    log_object_error_status(_log_context, _worker_id, ref->class_id, ref->get_primary_key(), status, "getting object instance");
                    return Result::Error(Code::Datastore_Unknown);
                }

                return Result::Ok(!status.IsNotFound());
            }
            ResultCode<std::optional<Buffer<ObjectInstanceProto>>> maybe_get_object_instance(const data::ObjectReferenceS &ref) override {
                using Result = ResultCode<std::optional<Buffer<ObjectInstanceProto>>>;

                auto object_instance = _buffer_pool->get_buffer<ObjectInstanceProto>();

                Status status = object_instance.with_internal_buffer<Status>([&](InternalBuffer &internal_buffer) {
                    return get_for_update(ref->get_object_instance_key(), internal_buffer);
                });

                if (!status.ok() && !status.IsNotFound()) {
                    //unknown error
                    log_object_error_status(_log_context, _worker_id, ref->class_id, ref->get_primary_key(), status, "getting object instance");
                    return Result::Error(Code::Datastore_Unknown);
                }
                if (status.IsNotFound()) {
                    return Result::Ok(std::nullopt);
                }
                if (object_instance.empty()) {
                    log_error(_log_context, "{0} object instance corrupt", get_class_log_context(_worker_id, ref->class_id, ref->get_primary_key()));
                    return Result::Error(Code::Datastore_ObjectInstanceCorrupted);
                }

                return Result::Ok(std::move(object_instance));
            }
            ResultCode<std::optional<Buffer<ObjectPropertiesIndexProto>>>
            maybe_get_object_properties_index(const data::ObjectReferenceS &ref) override {
                using Result = ResultCode<std::optional<Buffer<ObjectPropertiesIndexProto>>>;

                auto object_properties_index = _buffer_pool->get_buffer<ObjectPropertiesIndexProto>();
                Status status = object_properties_index.with_internal_buffer<Status>([&](InternalBuffer &internal_buffer) {
                    return get_for_update(ref->get_object_properties_index_key(), internal_buffer);
                });
                if (!status.ok() && !status.IsNotFound()) {
                    //unknown error
                    log_object_error_status(_log_context, _worker_id, ref->class_id, ref->get_primary_key(), status, "getting object properties index");
                    return Result::Error(Code::Datastore_Unknown);
                }
                if (status.IsNotFound()) {
                    return Result::Ok(std::nullopt);
                }
                if (object_properties_index.empty()) {
                    log_error(_log_context, "{0} object properties index corrupt",
                              get_class_log_context(_worker_id, ref->class_id, ref->get_primary_key()));
                    return Result::Error(Code::Datastore_ObjectPropertiesIndexCorrupted);
                }

                return Result::Ok(std::move(object_properties_index));
            }

            void undo_get_for_update(const std::string_view key) override {
                _txn->UndoGetForUpdate(_default_column_family, key);
            }
            ResultCode<std::optional<data::Cell>> maybe_get_cell(const std::string &property_key) override {
                using Result = ResultCode<std::optional<data::Cell>>;

                std::string slice_value{};
                auto buffer = this->_buffer_pool->get_buffer<CellProto>();

                Status get_s{};
                buffer.with_internal_buffer([&](estate::InternalBuffer &buff) {
                    get_s = get_for_update(property_key, buff);
                });

                if (!get_s.ok()) {
                    if (get_s.IsNotFound()) {
                        return Result::Ok(std::nullopt);
                    }
                    log_error(_log_context,
                              "worker id: {} property key: {} rocks db status: {} Failuring while getting cell",
                              _worker_id, property_key, get_s.ToString());
                    return Result::Error(Code::Datastore_Unknown);
                }

                return Result::Ok(std::move(buffer));
            }
            UnitResultCode write_cell(const data::CellView &cell_buffer, const std::string_view key) override {
                using Result = UnitResultCode;

                rocksdb::Slice cell_slice{cell_buffer.as_char(), cell_buffer.size()};
                auto put_s = _txn->Put(_default_column_family, key, cell_slice);
                if (!put_s.ok()) {
                    log_error(_log_context,
                              "worker id: {} property key: {} rocks db status: {} Failuring while writing cell",
                              _worker_id, key, put_s.ToString());
                    return Result::Error(Code::Datastore_Unknown);
                }

                return Result::Ok();
            }
            virtual UnitResultCode delete_value(const std::string_view key) override {
                using Result = UnitResultCode;
                auto delete_s = _txn->Delete(_default_column_family, key);
                if (!delete_s.ok()) {
                    log_error(_log_context,
                              "worker id: {} property key: {} rocks db status: {} Failuring while deleting property",
                              _worker_id, key, delete_s.ToString());
                    return Result::Error(Code::Datastore_Unknown);
                }
                return Result::Ok();
            }
            WorkerId get_worker_id() override {
                return _worker_id;
            }
            WorkerVersion get_worker_version() override {
                return _worker_version;
            }
            UnitResultCode delete_worker_index() override {
                using Result = UnitResultCode;

                auto delete_s = _txn->Delete(ESTATE_DB_WORKER_INDEX_KEY);
                if (!delete_s.ok()) {
                    log_worker_version_error_status(_log_context, _worker_id, _worker_version, delete_s, "deleting worker index");
                    return Result::Error(Code::Datastore_Unknown);
                }
                return Result::Ok();
            }
            UnitResultCode delete_engine_source() override {
                using Result = UnitResultCode;

                auto delete_s = _txn->Delete(ESTATE_DB_ENGINE_SOURCE_KEY);
                if (!delete_s.ok()) {
                    log_worker_version_error_status(_log_context, _worker_id, _worker_version, delete_s, "deleting engine data");
                    return Result::Error(Code::Datastore_Unknown);
                }
                return Result::Ok();
            }
            UnitResultCode save_worker_index(const WorkerVersion new_worker_version, const BufferView<WorkerIndexProto> &worker_index) override {
                using Result = UnitResultCode;

                auto worker_version_str = std::to_string(new_worker_version);
                auto put_worker_version_s = _txn->Put(ESTATE_DB_WORKER_VERSION_KEY, worker_version_str);
                if (!put_worker_version_s.ok()) {
                    log_worker_version_error_status(_log_context, _worker_id, _worker_version, put_worker_version_s, "putting worker version");
                }

                rocksdb::Slice slice(worker_index.as_char(), worker_index.size());
                auto put_index_s = _txn->Put(ESTATE_DB_WORKER_INDEX_KEY, slice);
                if (!put_index_s.ok()) {
                    log_worker_version_error_status(_log_context, _worker_id, _worker_version, put_index_s, "putting worker index");
                    return Result::Error(Code::Datastore_Unknown);
                }

                this->_worker_version = new_worker_version;

                return Result::Ok();
            }
            UnitResultCode commit() override {
                using Result = UnitResultCode;

                auto commit_s = _txn->Commit();
                if (!commit_s.ok()) {
                    if (commit_s.IsBusy())
                        return Result::Error(Code::Datastore_WriteConflictTryAgain);
                    log_worker_error_status(_log_context, _worker_id, commit_s, "committing transaction");
                    return Result::Error(Code::Datastore_Unknown);
                }
                return Result::Ok();
            }
            ResultCode<Buffer<WorkerIndexProto>, Code> get_worker_index() override {
                using Result = ResultCode<Buffer<WorkerIndexProto>, Code>;

                auto worker_index_buffer = _buffer_pool->get_buffer<WorkerIndexProto>();

                Status worker_index_s;
                worker_index_buffer.with_internal_buffer([&](InternalBuffer &buffer) {
                    worker_index_s = get_for_read(ESTATE_DB_WORKER_INDEX_KEY, buffer);
                });
                if (!worker_index_s.ok()) {
                    return Result::Error(Code::Datastore_MustGetLatestWorker);
                }

                if (worker_index_buffer.empty()) {
                    log_error(_log_context, "{0} worker index was empty", get_worker_version_log_context(_worker_id, _worker_version));
                    return Result::Error(Code::Datastore_WorkerIndexCorrupted);
                }

                return Result::Ok(std::move(worker_index_buffer));
            }
            UnitResultCode save_engine_source(BufferView<EngineSourceProto> engine_source) override {
                using Result = UnitResultCode;

                rocksdb::Slice slice(engine_source.as_char(), engine_source.size());
                auto put_s = _txn->Put(ESTATE_DB_ENGINE_SOURCE_KEY, slice);
                if (!put_s.ok()) {
                    log_worker_version_error_status(_log_context, _worker_id, _worker_version, put_s, "putting engine source");
                    return Result::Error(Code::Datastore_Unknown);
                }
                return Result::Ok();
            }
            ResultCode<Buffer<EngineSourceProto>> get_engine_source() override {
                using Result = ResultCode<Buffer<EngineSourceProto>>;

                auto engine_source = _buffer_pool->get_buffer<EngineSourceProto>();
                const auto status = engine_source.with_internal_buffer<Status>([this](InternalBuffer &buffer) {
                    return get_for_read(ESTATE_DB_ENGINE_SOURCE_KEY, buffer);
                });

                if (!status.ok()) {
                    return Result::Error(Code::Datastore_MustGetLatestWorker);
                }

                if (engine_source.empty()) {
                    log_error(_log_context, "{0} engine source was empty", get_worker_version_log_context(_worker_id, _worker_version));
                    return Result::Error(Code::Datastore_EngineSourceCorrupted);
                }

                return Result::Ok(std::move(engine_source));
            }
        };

        const rocksdb::WriteOptions TransactionImpl::WRITE_OPTIONS{}; // NOLINT(cert-err58-cpp)
        const rocksdb::ReadOptions TransactionImpl::READ_OPTIONS{}; // NOLINT(cert-err58-cpp)

        ResultCode<bool, Code> is_deleted(const LogContext &log_context, rocksdb::DB *db, const WorkerId &worker_id) {
            assert(db);
            using Result = ResultCode<bool, Code>;

            std::string buffer{};
            auto s = db->Get(rocksdb::ReadOptions(), "deleted", &buffer);
            if (!s.ok()) {
                if (s.IsNotFound())
                    return Result::Ok(false);
                log_worker_error_status(log_context, worker_id, s, "getting deleted");
                return Result::Error(Code::Datastore_Unknown);
            }

            return Result::Ok(buffer == "true");
        }
        class DatabaseImpl : public virtual IDatabase {
            static const rocksdb::ReadOptions READ_OPTIONS;
            static const rocksdb::WriteOptions WRITE_OPTIONS;
            const WorkerId worker_id;
            const std::string deleted_file;
            rocksdb::OptimisticTransactionDB *txn_db;
            rocksdb::DB *base_db;
            BufferPoolS buffer_pool;
            ResultCode<WorkerVersion, Code>
            get_worker_version_for_transaction(const LogContext &log_context, rocksdb::Transaction *txn, bool for_writable) {
                assert(txn);
                using Result = ResultCode<WorkerVersion, Code>;
                std::string worker_version_str;

                auto s = for_writable ?
                         txn->GetForUpdate(READ_OPTIONS, ESTATE_DB_WORKER_VERSION_KEY, &worker_version_str) :
                         txn->Get(READ_OPTIONS, ESTATE_DB_WORKER_VERSION_KEY, &worker_version_str);

                if (!s.ok()) {
                    log_worker_error_status(log_context, worker_id, s, "getting latest worker version number");
                    return Result::Error(Code::Datastore_Unknown);
                }

                if (worker_version_str.empty()) {
                    log_error(log_context, "{} {} was empty", get_worker_log_context(worker_id), ESTATE_DB_WORKER_VERSION_KEY);
                    return Result::Error(Code::Datastore_WorkerVersionCorrupted);
                }
                return Result::Ok(std::stoull(worker_version_str));
            }
        public:
            explicit DatabaseImpl(const WorkerId worker_id, std::string deleted_file, rocksdb::OptimisticTransactionDB *txn_db_, rocksdb::DB *base_db_,
                                  BufferPoolS buffer_pool) :
                    worker_id(worker_id), txn_db(txn_db_), base_db(base_db_), deleted_file(std::move(deleted_file)), buffer_pool(buffer_pool) {}
            ~DatabaseImpl() override {
                auto s = txn_db->Close();
                if (!s.ok()) {
                    sys_log_critical("RocksDb closed with error. Code {0}, Message {1}", s.code(), s.ToString());
                }
                delete txn_db;
                sys_log_trace("Database {0} closed", worker_id);
            }
        public:
            ResultCode<WorkerVersion, Code> get_worker_version(const LogContext &log_context) override {
                using Result = ResultCode<WorkerVersion, Code>;
                std::string worker_version_str;
                auto s = base_db->Get(READ_OPTIONS, ESTATE_DB_WORKER_VERSION_KEY, &worker_version_str);
                if (!s.ok()) {
                    log_worker_error_status(log_context, worker_id, s, "getting worker version number");
                    return Result::Error(Code::Datastore_Unknown);
                }
                if (worker_version_str.empty()) {
                    log_error(log_context, "{} {} was empty", get_worker_log_context(worker_id), ESTATE_DB_WORKER_VERSION_KEY);
                    return Result::Error(Code::Datastore_WorkerVersionCorrupted);
                }
                return Result::Ok(std::stoull(worker_version_str));
            }
            ResultCode<ITransactionS, Code> create_transaction(const LogContext &log_context, WorkerVersion worker_version) override {
                using Result = ResultCode<ITransactionS, Code>;

                auto *inner_txn = txn_db->BeginTransaction(TransactionImpl::WRITE_OPTIONS);

                UNWRAP_OR_RETURN(worker_version_comp, get_worker_version_for_transaction(log_context, inner_txn, true));
                if (worker_version_comp != worker_version)
                    return Result::Error(Code::Datastore_MustGetLatestWorker);

                return Result::Ok(std::make_shared<TransactionImpl>(log_context, inner_txn, base_db->DefaultColumnFamily(), worker_id, worker_version,
                                                                    buffer_pool));
            }
            ResultCode<Buffer<WorkerIndexProto>, Code> get_worker_index(const LogContext &log_context) override {
                using Result = ResultCode<Buffer<WorkerIndexProto>, Code>;

                auto worker_index_buffer = buffer_pool->get_buffer<WorkerIndexProto>();
                Status s;
                worker_index_buffer.with_internal_buffer([&](InternalBuffer &buffer) {
                    s = base_db->Get(READ_OPTIONS, ESTATE_DB_WORKER_INDEX_KEY, &buffer);
                });
                if (!s.ok()) {
                    if (s.IsNotFound())
                        return Result::Error(Code::Datastore_WorkerIndexNotFound);
                    log_worker_error_status(log_context, worker_id, s, "getting worker index");
                    return Result::Error(Code::Datastore_Unknown);
                }

                if (worker_index_buffer.empty()) {
                    log_error(log_context, "{0} worker index corrupted", get_worker_log_context(worker_id));
                    return Result::Error(Code::Datastore_WorkerIndexCorrupted);
                }

                return Result::Ok(std::move(worker_index_buffer));
            }
            ResultCode<Buffer<EngineSourceProto>, Code> get_engine_source(const LogContext &log_context) override {
                using Result = ResultCode<Buffer<EngineSourceProto>, Code>;

                auto engine_data_buffer = buffer_pool->get_buffer<EngineSourceProto>();
                Status s;
                engine_data_buffer.with_internal_buffer([&](InternalBuffer &buffer) {
                    s = base_db->Get(READ_OPTIONS, ESTATE_DB_ENGINE_SOURCE_KEY, &buffer);
                });
                if (!s.ok()) {
                    if (s.IsNotFound())
                        return Result::Error(Code::Datastore_EngineSourceNotFound);
                    log_worker_error_status(log_context, worker_id, s, "getting engine source");
                    return Result::Error(Code::Datastore_Unknown);
                }

                if (engine_data_buffer.empty()) {
                    log_error(log_context, "{0} engine source corrupted", get_worker_log_context(worker_id));
                    return Result::Error(Code::Datastore_EngineSourceCorrupted);
                }

                return Result::Ok(std::move(engine_data_buffer));
            }
            UnitResultCode mark_as_deleted(const LogContext &log_context) override {
                using Result = UnitResultCode;

                auto s = base_db->Put(WRITE_OPTIONS, "deleted", "true");
                if (!s.ok()) {
                    log_worker_error_status(log_context, worker_id, s, "marking as deleted");
                    return Result::Error(Code::Datastore_Unknown);
                }

                if (!touch(log_context, deleted_file)) {
                    log_critical(log_context, "{0} unable to Create file which indicates database has been deleted {1}",
                                 get_worker_log_context(worker_id), deleted_file);
                    return Result::Error(Code::Datastore_UnknownSystemError);
                }

                return Result::Ok();
            }
        };

        const rocksdb::ReadOptions DatabaseImpl::READ_OPTIONS{};
        const rocksdb::WriteOptions DatabaseImpl::WRITE_OPTIONS{};

        std::mutex *DatabaseManager::get_and_lock_open_databases_mutex(const WorkerId worker_id) {
            std::lock_guard<std::mutex> lock(open_databases_mutexes_mutex);
            auto *worker_id_mutex = &open_database_mutexes[worker_id];
            worker_id_mutex->lock();
            return worker_id_mutex;
        }

        ResultCode<IDatabaseS, Code> DatabaseManager::get_database(const LogContext &log_context, const WorkerId worker_id, bool is_new,
                                                                   std::optional<WorkerVersion> initial_worker_version) {
            using Result = ResultCode<IDatabaseS, Code>;

            assert(is_new == initial_worker_version.has_value());

            auto get_db_r = try_get_database(worker_id);
            if (get_db_r) {
                return Result::Ok(get_db_r.unwrap());
            }

            std::mutex *worker_id_mutex = get_and_lock_open_databases_mutex(worker_id);
            IDatabaseS db{};
            auto open_db_r = open_database(log_context, worker_id, is_new, initial_worker_version);
            if (!open_db_r) {
                worker_id_mutex->unlock();
                return Result::Error(open_db_r.get_error());
            }
            db = open_db_r.unwrap();
            std::lock_guard<std::mutex> lock(databases_mutex);
            databases[worker_id] = db;
            worker_id_mutex->unlock();
            return Result::Ok(std::move(db));
        }

        DatabaseManager::DatabaseManager(DatabaseManagerConfiguration config, BufferPoolS buffer_pool) :
                config(std::move(config)), buffer_pool_service(std::move(buffer_pool)) {}

        ResultCode<IDatabaseS, Code> DatabaseManager::open_database(const LogContext &log_context, const WorkerId worker_id, bool is_new,
                                                                    std::optional<WorkerVersion> initial_worker_version) {
            using Result = ResultCode<IDatabaseS, Code>;
            assert(is_new == initial_worker_version.has_value());

            auto deleted_file = fmt::format(config.deleted_file_format, worker_id);
            if (boost::filesystem::exists(deleted_file)) {
                log_warn(log_context, "{0} attempted to open database when deleted file exists", get_worker_log_context(worker_id));
                return Result::Error(Code::Datastore_DeletedFileExists);
            }

            auto wal_dir = fmt::format(config.wal_dir_format, worker_id);
            auto data_dir = fmt::format(config.data_dir_format, worker_id);

            if (is_new) {
                boost::system::error_code ec;
                boost::filesystem::create_directories(wal_dir, ec);
                if (ec.failed()) {
                    log_critical(log_context, "Unable to Create database wal directory {0} because {1}", wal_dir, ec.message());
                    return Result::Error(Code::Datastore_UnableToCreateDirectory);
                }
                boost::filesystem::create_directories(data_dir, ec);
                if (ec.failed()) {
                    log_critical(log_context, "Unable to Create database data directory {0} because {1}", data_dir, ec.message());
                    return Result::Error(Code::Datastore_UnableToCreateDirectory);
                }
            }

            rocksdb::OptimisticTransactionDB *txn_db = nullptr;
            rocksdb::Options options;
            options.create_if_missing = is_new;
            options.wal_dir = wal_dir;

            if (this->config.optimize_for_small_db)
                options.OptimizeForSmallDb();

            auto s = rocksdb::OptimisticTransactionDB::Open(options, data_dir, &txn_db);
            if (!s.ok()) {
                log_error(log_context, "Failed to open database, error code {0}, message {1}", s.code(), s.ToString());
                return Result::Error(Code::Datastore_UnableToOpen);
            }
            rocksdb::DB *base_db = txn_db->GetBaseDB();

            //Set the initial worker version so new transactions can be created
            if (is_new) {
                static const rocksdb::WriteOptions WRITE_OPTIONS{};
                auto worker_version = initial_worker_version.value();
                auto put_s = base_db->Put(WRITE_OPTIONS, ESTATE_DB_WORKER_VERSION_KEY, std::to_string(worker_version));
                if (!put_s.ok()) {
                    log_worker_error_status(log_context, worker_id, put_s, "Setting initial worker version");
                    return Result::Error(Code::Datastore_FailedToSetInitialWorkerVersion);
                }
            }

            auto is_deleted_r = is_deleted(log_context, base_db, worker_id);
            if (!is_deleted_r) {
                log_error(log_context,
                          "Failed to open database due to not being able to read from it while seeing if the deleted flag there, error code {0}, message {1}",
                          s.code(), s.ToString());
                return Result::Error(Code::Datastore_Unknown);
            }
            auto is_deleted = is_deleted_r.unwrap();

            if (is_deleted) {
                txn_db->Close();
                delete txn_db;
                log_warn(log_context, "{0} attempted to open database that contains deleted flag", get_worker_log_context(worker_id));
                return Result::Error(Code::Datastore_DeletedFlagExists);
            }

            auto obj_database = std::make_shared<DatabaseImpl>(worker_id, deleted_file, txn_db, base_db, buffer_pool_service.get_service());
            return Result::Ok(std::move(obj_database));
        }

        Result<IDatabaseS> DatabaseManager::try_get_database(const WorkerId worker_id) {
            using Result = Result<IDatabaseS>;
            std::lock_guard<std::mutex> lock(databases_mutex);
            auto found = databases.find(worker_id);
            if (found != databases.end()) {
                auto db = found->second;
                return Result::Ok(std::move(db));
            }
            return Result::Error();
        }
        void DatabaseManager::close_database(const LogContext &log_context, const WorkerId worker_id) {
            auto *worker_id_mutex = get_and_lock_open_databases_mutex(worker_id);
            {
                std::lock_guard<std::mutex> lock(databases_mutex);
                auto found = databases.find(worker_id);
                if (found != databases.end()) {
                    auto db = found->second;
                    auto use_count = db.use_count();
                    if (use_count > 1) {
                        log_warn(log_context, "There were {0} uses of {1} database when closing database", use_count, worker_id);
                    }
                }
                databases.erase(worker_id);
            }
            {
                std::lock_guard<std::mutex> lock2(open_databases_mutexes_mutex);
                worker_id_mutex->unlock();
                open_database_mutexes.erase(worker_id);
            }
        }
        const DatabaseManagerConfiguration &DatabaseManager::get_config() {
            return config;
        }
    }
    namespace engine {
        CallContext::CallContext(const LogContext &log_context, storage::ITransactionS txn, BufferPoolS buffer_pool, bool cached_working_set) :
                _log_context{log_context}, _txn{std::move(txn)}, _buffer_pool{std::move(buffer_pool)}, _cached_working_set{cached_working_set} {}
        storage::ITransactionS CallContext::get_transaction() const {
            return _txn;
        }
        data::WorkingSetS CallContext::get_working_set() {
            if (!_maybe_working_set.has_value())
                _maybe_working_set.emplace(std::make_shared<data::WorkingSet>(_cached_working_set, this->shared_from_this()));
            return _maybe_working_set.value();
        }
        std::optional<std::vector<Buffer<DataDeltaProto>>> CallContext::extract_deltas() {
            return std::move(_maybe_deltas);
        }
        void CallContext::add_delta(Buffer<DataDeltaProto> delta) {
            if (!_maybe_deltas.has_value()) {
                _maybe_deltas.emplace();
            }
            _maybe_deltas->emplace_back(std::move(delta));
        }
        std::optional<std::vector<Buffer<MessageProto>>> CallContext::extract_fired_events() {
            return std::move(_maybe_fired_events);
        }
        void CallContext::add_fired_event(Buffer<MessageProto> event) {
            if (!_maybe_fired_events.has_value()) {
                _maybe_fired_events.emplace();
            }
            _maybe_fired_events->emplace_back(std::move(event));
        }
        const LogContext &CallContext::get_log_context() const {
            return _log_context;
        }
        fbs::BuilderS CallContext::get_reusable_builder(bool clear) {
            if (!_maybe_reusable_builder.has_value())
                _maybe_reusable_builder.emplace(std::make_shared<flatbuffers::FlatBufferBuilder>());

            if (clear)
                _maybe_reusable_builder.value()->Clear();

            return _maybe_reusable_builder.value();
        }
        BufferPoolS CallContext::get_buffer_pool() const {
            return _buffer_pool;
        }
        ConsoleLogS CallContext::get_console_log() {
            if (!_maybe_console_log)
                _maybe_console_log.emplace(std::make_shared<ConsoleLog>());
            return _maybe_console_log.value();
        }
        v8::Isolate *CallContext::get_isolate() const {
            assert(_maybe_isolate.has_value());
            return _maybe_isolate.value();
        }
        void CallContext::set_isolate(v8::Isolate *isolate) {
            assert(isolate != nullptr);
            _maybe_isolate = isolate;
        }
        CallContext::~CallContext() {}
        void ConsoleLog::append_log(const std::string message) {
            if (!_maybe_messages.has_value()) {
                _maybe_messages.emplace();
            }
            _maybe_messages.value().push_back(std::make_pair(std::move(message), false));
        }
        void ConsoleLog::append_error(const std::string message) {
            if (!_maybe_messages.has_value()) {
                _maybe_messages.emplace();
            }
            _maybe_messages.value().push_back(std::make_pair(std::move(message), true));
        }
        const std::optional<std::vector<std::pair<std::string, bool>>> &ConsoleLog::maybe_get_messages() {
            return _maybe_messages;
        }
        std::optional<Buffer<ConsoleLogProto>> create_console_log_proto(BufferPoolS buffer_pool, ConsoleLogS console_log) {
            flatbuffers::FlatBufferBuilder console_log_builder{};

            std::vector<fbs::Offset<ConsoleLogMessageProto>> log_message_offsets{};
            auto &maybe_messages = console_log->maybe_get_messages();
            if (maybe_messages.has_value() && !maybe_messages.value().empty()) {
                auto &messages = maybe_messages.value();
                for (auto &[message, error]: messages) {
                    log_message_offsets.push_back(CreateConsoleLogMessageProto(console_log_builder,
                                                                               console_log_builder.CreateString(message),
                                                                               error));
                }
            }

            if (!log_message_offsets.empty()) {
                return finish_and_copy_to_buffer(console_log_builder, buffer_pool,
                                                 CreateConsoleLogProto(console_log_builder,
                                                                       console_log_builder.CreateVector(log_message_offsets)));
            }

            return std::nullopt;
        }
        EngineError::EngineError(EngineError::ErrorVariant c) :
                _error{std::move(c)} {
        }
        EngineError::EngineError(enum Code c) :
                EngineError(ErrorVariant{c}) {}
        EngineError::EngineError(ScriptException ex) :
                EngineError(ErrorVariant{std::move(ex)}) {}

        namespace javascript {
            struct ObjectFactoryFunctions {
                std::map<ClassId, v8::Global<v8::Function>> graft_functions;
                std::map<ClassId, v8::Global<v8::Function>> new_functions;
            };
            using ObjectFactoryFunctionsU = std::unique_ptr<ObjectFactoryFunctions>;
            class Engine {
                std::optional<Code> _internal_error;
                WorkerVersion _worker_version;
                CallContextW _call_context;
                Buffer<WorkerIndexProto> _worker_index;
                v8::Isolate *_isolate;
                v8::Global<v8::Context> _parent_context{};
                v8::ArrayBuffer::Allocator *_allocator;
                std::optional<std::map<ClassId, std::pair<std::string_view, data::ClassType>>> _maybe_class_name_types{};
                std::optional<std::map<std::string_view, ClassId>> _maybe_class_ids{};
                std::optional<std::set<std::string>> _maybe_passthrough_class_name_holder{};
                std::map<ClassId, std::optional<std::set<std::string>>> _service_method_sets{};
                std::map<ClassId, std::optional<std::multimap<std::string, MethodKindProto>>> _object_method_maps{};
                ObjectFactoryFunctionsU _object_factory_functions;
                bool _moved{false};
            public:
                Engine(Buffer<WorkerIndexProto> worker_index,
                       v8::Isolate *isolate, v8::ArrayBuffer::Allocator *allocator,
                       v8::Local<v8::Context> parent_context,
                       ObjectFactoryFunctionsU object_factory_functions) :
                        _worker_version(worker_index->worker_version()),
                        _internal_error(std::nullopt),
                        _worker_index(std::move(worker_index)),
                        _allocator{allocator},
                        _isolate{isolate},
                        _object_factory_functions{std::move(object_factory_functions)} {
                    parent_context->SetEmbedderData(0, v8::External::New(isolate, this));
                    _parent_context.Reset(_isolate, parent_context);
                }
                Engine(const Engine &other) = delete;
                Engine(Engine &&other) noexcept:
                        _worker_version(other._worker_version),
                        _allocator(other._allocator),
                        _isolate(other._isolate),
                        _parent_context(std::move(other._parent_context)),
                        _call_context(std::move(other._call_context)),
                        _worker_index(std::move(other._worker_index)),
                        _internal_error(std::move(other._internal_error)),
                        _service_method_sets(std::move(other._service_method_sets)),
                        _object_method_maps(std::move(other._object_method_maps)),
                        _moved(false) {
                    other._moved = true;
                }
                ~Engine() {
                    if (!_moved) {
                        _object_factory_functions.release();
                        _parent_context.Reset();
                        _isolate->Dispose();
                        _isolate = nullptr;
                        delete _allocator;
                    }
                }
                template<data::ObjectType OT>
                std::optional<ClassId> _get_derived_class_id(v8::Isolate *isolate_, const v8::Local<v8::Value> &value) {
                    V8_SCOPE(isolate_);
                    if (value->IsNull() || !value->IsObject())
                        return std::nullopt;
                    const auto object = v8::Local<v8::Object>::Cast(value);
                    const auto ctor_name = value_to_string(isolate, object->GetConstructorName());
                    auto class_id_v = get_class_id<data::get_class_type<OT>()>(ctor_name);
                    if (std::holds_alternative<ClassId>(class_id_v)) {
                        return std::get<ClassId>(class_id_v);
                    } else {
                        switch (std::get<ClassLookupCode>(class_id_v)) {
                            case ClassLookupCode::NOT_FOUND:
                                return _get_derived_class_id<OT>(isolate, object->GetPrototype());
                            default:
                                assert(false); //If this hits it means the object/service got stored as the wrong type.
                                return std::nullopt;
                        }
                    }
                }
                const v8::Global<v8::Function> &get_graft_function(ClassId class_id) const {
                    const auto it = _object_factory_functions->graft_functions.find(class_id);
                    assert(it != _object_factory_functions->graft_functions.cend()); //we know the class_id so it should always be found.
                    return it->second;
                }
                const v8::Global<v8::Function> &get_new_function(ClassId class_id) const {
                    const auto it = _object_factory_functions->new_functions.find(class_id);
                    assert(it != _object_factory_functions->new_functions.cend());
                    return it->second;
                }
                template<data::ObjectType OT>
                std::optional<ClassId> maybe_get_derived_class_id(v8::Isolate *isolate, const v8::Local<v8::Object> &object) {
                    return _get_derived_class_id<OT>(isolate, object);
                }
                void clear_internal_error() {
                    assert(!_moved);
                    _internal_error = std::nullopt;
                }
                void set_internal_error(Code code) {
                    assert(!_moved);
                    _internal_error = code;
                }
                std::optional<Code> get_internal_error() const {
                    assert(!_moved);
                    return _internal_error;
                }
                const WorkerIndexProto &get_worker_index() const {
                    assert(!_moved);
                    return *_worker_index.get_flatbuffer();
                }
                void set_call_context(CallContextS call_context) {
                    assert(!_moved);
                    _call_context = call_context;
                }
                [[nodiscard]] CallContextS get_call_context() {
                    assert(!_moved);
                    return _call_context.lock();
                }
                v8::Isolate *isolate() {
                    assert(!_moved);
                    return _isolate;
                }
                v8::Global<v8::Context> &parent_context() {
                    assert(!_moved);
                    return _parent_context;
                }
                [[nodiscard]] WorkerVersion worker_version() const {
                    assert(!_moved);
                    return _worker_version;
                }
                [[nodiscard]] bool service_has_method(ClassId class_id, const std::string &name) {
                    const auto methods = get_service_methods(class_id);
                    if (!methods)
                        return false;
                    return methods->find(name) != methods->end();
                }
                [[nodiscard]] bool object_has_method(ClassId class_id, const std::string &name) {
                    const auto methods = get_object_methods(class_id);
                    if (!methods)
                        return false;
                    auto it = methods->find(name);
                    return it != methods->end();
                }
                [[nodiscard]] bool object_has_setter_method(ClassId class_id, const std::string &name) {
                    const auto methods = get_object_methods(class_id);
                    if (!methods)
                        return false;
                    auto it = methods->find(name);
                    while (it != methods->end()) {
                        if (it->second == MethodKindProto::Setter)
                            return true;
                        ++it;
                    }
                    return false;
                }
                [[nodiscard]] std::optional<MethodKindProto> object_has_getter_or_normal_method(ClassId class_id, const std::string &name) {
                    const auto methods = get_object_methods(class_id);
                    if (!methods)
                        return std::nullopt;
                    auto it = methods->find(name);
                    while (it != methods->end()) {
                        if (it->second == MethodKindProto::Getter)
                            return MethodKindProto::Getter;
                        if (it->second == MethodKindProto::Normal)
                            return MethodKindProto::Normal;
                        ++it;
                    }
                    return std::nullopt;
                }
                template<data::ClassType CT>
                [[nodiscard]] std::variant<ClassId, ClassLookupCode> get_class_id(const std::string_view class_name) {
                    if (!has_populated_class_names())
                        populate_class_names();
                    const auto &class_ids = _maybe_class_ids.value();
                    const auto it = class_ids.find(class_name);
                    if (it != class_ids.end()) {
                        const auto class_type = _maybe_class_name_types->find(it->second)->second.second;
                        if (class_type != CT)
                            return ClassLookupCode::WRONG_CLASS_TYPE;
                        return it->second;
                    }
                    return ClassLookupCode::NOT_FOUND;
                }
                std::optional<std::pair<std::string_view, data::ClassType>> maybe_get_class_name_type(ClassId class_id) {
                    if (!has_populated_class_names())
                        populate_class_names();
                    const auto &class_name_types = _maybe_class_name_types.value();
                    const auto it = class_name_types.find(class_id);
                    if (it != class_name_types.end()) {
                        return it->second;
                    }
                    return std::nullopt;
                }
            private:
                bool has_populated_class_names() const {
                    return _maybe_class_name_types.has_value() && _maybe_class_ids.has_value() && _maybe_passthrough_class_name_holder.has_value();
                }
                void populate_class_names() {
                    const auto &worker_index = this->get_worker_index();

                    _maybe_class_name_types.emplace();
                    _maybe_class_ids.emplace();
                    _maybe_passthrough_class_name_holder.emplace();

                    auto &class_name_types = _maybe_class_name_types.value();
                    auto &class_ids = _maybe_class_ids.value();
                    auto &passthrough_class_name_holder = _maybe_passthrough_class_name_holder.value();

                    if (worker_index.service_classes() && worker_index.service_classes()->size()) {
                        for (auto i = 0; i < worker_index.service_classes()->size(); ++i) {
                            const auto cls = worker_index.service_classes()->Get(i);
                            auto id = cls->class_id();
                            auto name = cls->class_name()->string_view();
                            class_name_types[id] = std::make_pair(name, data::ClassType::WORKER_SERVICE);
                            class_ids[name] = id;
                            //store the passthrough class name as well as the regular class name so the class id can be looked up using it.
                            const auto[at, _] = passthrough_class_name_holder.insert(fmt::format(ESTATE_PASSTHROUGH_CLASS_NAME_FORMAT, name));
                            class_ids[*at] = id;
                        }
                    }
                    if (worker_index.data_classes() && worker_index.data_classes()->size()) {
                        for (auto i = 0; i < worker_index.data_classes()->size(); ++i) {
                            const auto cls = worker_index.data_classes()->Get(i);
                            auto name = cls->class_name()->string_view();
                            auto id = cls->class_id();
                            class_name_types[id] = std::make_pair(name, data::ClassType::WORKER_OBJECT);
                            class_ids[name] = id;
                            //store the passthrough class name as well as the regular class name so the class id can be looked up using it.
                            const auto[at, _] = passthrough_class_name_holder.insert(fmt::format(ESTATE_PASSTHROUGH_CLASS_NAME_FORMAT, name));
                            class_ids[*at] = id;
                        }
                    }
                    if (worker_index.message_classes() && worker_index.message_classes()->size()) {
                        for (auto i = 0; i < worker_index.message_classes()->size(); ++i) {
                            const auto cls = worker_index.message_classes()->Get(i);
                            auto name = cls->class_name()->string_view();
                            auto id = cls->class_id();
                            class_name_types[id] = std::make_pair(name, data::ClassType::WORKER_EVENT);
                            class_ids[name] = id;
                            //note: events don't use passthrough classes
                        }
                    }
                }
                const std::set<std::string> *get_service_methods(ClassId class_id) {
                    auto it = _service_method_sets.find(class_id);
                    if (it != _service_method_sets.end()) {
                        return it->second.has_value() ?
                               &it->second.value() :
                               nullptr;
                    }

                    const auto &worker_index = this->get_worker_index();

                    if (!worker_index.service_classes() || !worker_index.service_classes()->size()) {
                        _service_method_sets[class_id] = std::nullopt;
                        return nullptr;
                    }

                    std::set<std::string> methods{};
                    for (auto c = 0; c < worker_index.service_classes()->size(); ++c) {
                        auto clazz = worker_index.service_classes()->Get(c);

                        if (clazz->class_id() == class_id) {
                            if (clazz->methods() && clazz->methods()->size()) {
                                for (auto m = 0; m < clazz->methods()->size(); ++m) {
                                    auto method = clazz->methods()->Get(m);
                                    std::string name = method->method_name()->str();
                                    methods.insert(std::move(name));
                                }
                            }
                            break;
                        }
                    }

                    _service_method_sets[class_id] = std::move(methods);
                    return &_service_method_sets[class_id].value();
                }
                const std::multimap<std::string, MethodKindProto> *get_object_methods(ClassId class_id) {
                    auto it = _object_method_maps.find(class_id);
                    if (it != _object_method_maps.end()) {
                        return it->second.has_value() ?
                               &it->second.value() :
                               nullptr;
                    }

                    const auto &worker_index = this->get_worker_index();

                    if (!worker_index.data_classes() || !worker_index.data_classes()->size()) {
                        _object_method_maps[class_id] = std::nullopt;
                        return nullptr;
                    }

                    std::multimap<std::string, MethodKindProto> methods{};
                    for (auto c = 0; c < worker_index.data_classes()->size(); ++c) {
                        auto clazz = worker_index.data_classes()->Get(c);

                        if (clazz->class_id() == class_id) {
                            if (clazz->methods() && clazz->methods()->size()) {
                                for (auto m = 0; m < clazz->methods()->size(); ++m) {
                                    auto method = clazz->methods()->Get(m);
                                    methods.insert(std::make_pair(
                                            method->method_name()->str(),
                                            method->method_kind()
                                    ));
                                }
                            }
                            break;
                        }
                    }

                    _object_method_maps[class_id] = std::move(methods);
                    return &_object_method_maps[class_id].value();
                }
            };
            struct GeneratedFactoryCode {
                std::string file_name;
                std::string code;
            };
            struct ModuleDef {
                bool instanciating{false};
                std::string_view file_name;
                std::string directory;
                std::string module_name;
                v8::Global<v8::Module> module;
            };
            class ModuleCache {
                v8::Isolate *_isolate;
                std::map<std::string_view, int> _core_module_name_identity_hash{};
                std::map<std::string_view, int> _user_module_name_identity_hash{};
                std::map<std::string_view, int> _file_name_identity_hash{};
                std::map<int, ModuleDef> _identity_hash_module_def;
            public:
                ModuleCache(v8::Isolate *isolate) :
                        _isolate{isolate} {}
                ModuleCache(const ModuleCache &) = delete;
                ModuleCache(ModuleCache &&) = delete;
                /*NOTE: For efficiency this modifies the cache in-place and will make the cache completely invalid if it returns an error.
                 * So it this returns an error: it's all done. Throw an exception.*/
                [[nodiscard]] UnitResultCode add_module(bool core, std::string_view file_name, v8::Local<v8::Module> module) {
                    using Result = UnitResultCode;
                    std::filesystem::path module_name;
                    if (core) {
                        module_name = file_name;
                    } else {
                        module_name = "/";
                        module_name.append(file_name);
                    }
                    module_name.replace_extension();
                    module_name = module_name.lexically_normal();
                    std::filesystem::path directory{"/"};
                    directory += file_name;
                    directory.remove_filename();
                    directory = directory.lexically_normal();
                    std::string dir_str{directory};
                    auto ident_hash = module->GetIdentityHash();
                    _file_name_identity_hash[file_name] = ident_hash;
                    _identity_hash_module_def.insert(std::make_pair(
                            std::move(ident_hash),
                            ModuleDef{
                                    false,
                                    file_name,
                                    std::move(directory),
                                    std::move(module_name),
                                    v8::Global<v8::Module>{_isolate, module}
                            }
                    ));
                    std::string_view module_name_view{_identity_hash_module_def[ident_hash].module_name};
                    if (core) {
                        _core_module_name_identity_hash[module_name_view] = ident_hash; //no dupes because I'm not a fucking idiot.
                    } else {
                        auto[_, inserted] = _user_module_name_identity_hash.insert(std::make_pair(
                                std::move(module_name_view),
                                std::move(ident_hash)
                        ));
                        if (!inserted) //duplicate module name (i.e. different extensions, same base filename)
                            return Result::Error(Code::ScriptEngine_DuplicateModuleName);
                    }
                    return Result::Ok();
                }
                std::map<int, ModuleDef> &get_module_defs() {
                    return _identity_hash_module_def;
                }
                std::string_view get_module_name(std::string_view file_name) {
                    const auto ident_it = _file_name_identity_hash.find(file_name);
                    assert(ident_it != _file_name_identity_hash.cend());
                    const auto module_def_it = _identity_hash_module_def.find(ident_it->second);
                    assert(module_def_it != _identity_hash_module_def.cend());
                    return module_def_it->second.module_name;
                }
                std::optional<std::reference_wrapper<ModuleDef>> maybe_resolve(int referrer_ident, std::string_view specifier) {
                    {
                        //if it's a core module
                        const auto core_ident_it = _core_module_name_identity_hash.find(specifier);
                        if (core_ident_it != _core_module_name_identity_hash.cend()) {
                            auto module_def_it = _identity_hash_module_def.find(core_ident_it->second);
                            assert(module_def_it != _identity_hash_module_def.end());
                            return module_def_it->second;
                        }
                    }

                    std::filesystem::path specifier_path{specifier};
                    specifier_path = specifier_path.lexically_normal();
                    specifier_path.replace_extension();

                    if (specifier_path.is_relative()) {
                        //make it absolute
                        const auto it = _identity_hash_module_def.find(referrer_ident);
                        assert(it != _identity_hash_module_def.cend());
                        const auto &referrer_module_def = it->second;
                        specifier_path = referrer_module_def.directory / specifier_path;
                    }

                    assert(specifier_path.is_absolute());
                    specifier_path = std::filesystem::weakly_canonical(specifier_path);
                    {
                        std::string specifier_path_str{specifier_path.string()};
                        const auto user_ident_it = _user_module_name_identity_hash.find(specifier_path_str);
                        if (user_ident_it == _user_module_name_identity_hash.cend())
                            return std::nullopt; //not found
                        const auto module_def_it = _identity_hash_module_def.find(user_ident_it->second);
                        assert(module_def_it != _identity_hash_module_def.cend());
                        return module_def_it->second;
                    }
                }
            };

            /* V8 ArrayBuffer allocator that disables all allocation. */
            class DisabledArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
            public:
                virtual ~DisabledArrayBufferAllocator() = default;
                virtual void* Allocate(size_t length) override {
                    return 0;
                }
                virtual void* AllocateUninitialized(size_t length) override {
                    return 0;
                }
                virtual void Free(void* data, size_t length) override {
                }
            };
            class EngineFactory {
                static v8::MaybeLocal<v8::Module>
                ResolveCallback(v8::Local<v8::Context> context, v8::Local<v8::String> specifier, v8::Local<v8::Module> referrer) {
                    auto isolate = context->GetIsolate();
                    static const char *FROM = "ResolveCallback";

                    v8::EscapableHandleScope handle_scope(isolate);
                    v8::Context::Scope context_scope{context};

                    v8::Handle<v8::External> embedder_data = v8::Handle<v8::External>::Cast(context->GetEmbedderData(0));
                    auto &module_cache = *static_cast<ModuleCache *>(embedder_data->Value());

                    std::string specifier_str = value_to_string(isolate, specifier);

                    auto maybe_module_def = module_cache.maybe_resolve(referrer->GetIdentityHash(), specifier_str);
                    if (!maybe_module_def.has_value()) {
                        V8_THROW_FMT(FROM, "Module import error: Unable to resolve {}", specifier_str);
                        return v8::MaybeLocal<v8::Module>();
                    }
                    auto &module_def = maybe_module_def.value().get();

                    auto module = module_def.module.Get(isolate);
                    const auto &file_name = module_def.file_name;

                    auto status = module->GetStatus();

                    //already instanciated, OK
                    if (status == v8::Module::kInstantiated || status == v8::Module::kEvaluated)
                        return v8::MaybeLocal<v8::Module>(handle_scope.Escape(module));

                    //Module cycle = bad
                    if (module_def.instanciating) {
                        V8_THROW_FMT(FROM,
                                     "Module cycle detected: Module {} imported a module that directly or indirectly caused {} to be imported again before it was finished importing.",
                                     file_name, file_name);
                        return v8::MaybeLocal<v8::Module>();
                    }

                    module_def.instanciating = true; //to catch cycles

                    //instanciate it
                    v8::TryCatch try_catch{isolate};
                    auto unused = module->InstantiateModule(context, &EngineFactory::ResolveCallback);

                    module_def.instanciating = false;

                    if (try_catch.HasCaught()) {
                        try_catch.ReThrow();
                        return v8::MaybeLocal<v8::Module>();
                    }
                    if (module->GetStatus() != v8::Module::kInstantiated) {
                        V8_THROW_FMT(FROM, "Unable to instanciate module {}", file_name);
                        return v8::MaybeLocal<v8::Module>();
                    }
                    return v8::MaybeLocal<v8::Module>(handle_scope.Escape(module));
                }
                static const std::string &GetTemplateCode(const std::map<std::string, std::string> &base_replacements, const std::string &path) {
                    static std::map<std::string, std::string> template_code{};
                    static std::mutex template_code_mutex{};
                    if (!template_code.contains(path)) {
                        std::lock_guard<std::mutex> template_code_lck{template_code_mutex};
                        if (!template_code.contains(path)) {
                            const auto js_rc = cmrc::estate::js::rc::get_filesystem();
                            const auto file = js_rc.open(path);
                            std::string template_{file.cbegin(), file.cend()};
                            for (const auto&[k, v]: base_replacements) {
                                boost::replace_all(template_, k, v);
                            }
                            template_code[path] = std::move(template_);
                        }
                    }
                    return template_code[path];
                }
#define base_repl(x) "@@@" x "@@@"
                static const std::string &GetDataFactoryImportsTemplate() {
                    static const std::map<std::string, std::string> base_replacements{
                            {base_repl("EXISTING_WORKER_OBJECT_CTOR"), ESTATE_EXISTING_DATA_CTOR_STR},
                            {base_repl("SERVER_INTERNAL_MODULE"),    ESTATE_KERNEL_INTERNAL_MODULE_NAME}
                    };
                    static const std::string path{"js/Data-factory-imports.js.in"};
                    return GetTemplateCode(base_replacements, path);
                }
                static const std::string &GetDataFactoryTemplate() {
                    static const std::map<std::string, std::string> base_replacements{
                            {base_repl("EXISTING_WORKER_OBJECT_CTOR"),     ESTATE_EXISTING_DATA_CTOR_STR},
                            {base_repl("PASSTHROUGH_CLASS_NAME_PREFIX"), ESTATE_PASSTHROUGH_CLASS_NAME_PREFIX},
                            {base_repl("GRAFT_FUNC_PREFIX"),             ESTATE_GRAFT_FUNC_PREFIX}
                    };
                    static const std::string path{"js/Data-factory.js.mustache"};
                    return GetTemplateCode(base_replacements, path);
                }
                static const std::string &GetServiceFactoryImportsTemplate() {
                    static const std::map<std::string, std::string> base_replacements{
                            {base_repl("EXISTING_WORKER_SERVICE_CTOR"), ESTATE_EXISTING_SERVICE_CTOR_STR},
                            {base_repl("SERVER_INTERNAL_MODULE"),     ESTATE_KERNEL_INTERNAL_MODULE_NAME}
                    };
                    static const std::string path{"js/Service-factory-imports.js.in"};
                    return GetTemplateCode(base_replacements, path);
                }
                static const std::string &GetServiceFactoryTemplate() {
                    static const std::map<std::string, std::string> base_replacements{
                            {base_repl("EXISTING_WORKER_SERVICE_CTOR"),    ESTATE_EXISTING_SERVICE_CTOR_STR},
                            {base_repl("PASSTHROUGH_CLASS_NAME_PREFIX"), ESTATE_PASSTHROUGH_CLASS_NAME_PREFIX},
                            {base_repl("GRAFT_FUNC_PREFIX"),             ESTATE_GRAFT_FUNC_PREFIX},
                            {base_repl("NEW_FUNC_PREFIX"),               ESTATE_NEW_FUNC_PREFIX}
                    };
                    static const std::string path{"js/Service-factory.js.mustache"};
                    return GetTemplateCode(base_replacements, path);
                }
#undef base_repl
                static GeneratedFactoryCode GenerateRuntimeCode(const BufferView<WorkerIndexProto> worker_index, ModuleCache &module_cache) {

                    const auto data_count = worker_index->data_classes() ? worker_index->data_classes()->size() : 0;
                    const auto service_count = worker_index->service_classes() ? worker_index->service_classes()->size() : 0;

                    assert(data_count || service_count); //must have one or both
                    assert(worker_index->file_names() && worker_index->file_names()->size()); //must have some files

                    GeneratedFactoryCode gen{};

                    //build a mapping of file_id to module_name
                    std::unordered_map<FileId, std::string_view> file_id_module_name{};
                    for (int i = 0; i < worker_index->file_names()->size(); ++i) {
                        const auto file = worker_index->file_names()->Get(i);
                        auto module_name = module_cache.get_module_name(file->file_name()->string_view());
                        file_id_module_name[file->file_name_id()] = module_name;
                    }

                    gen.file_name = "estate-factory.mjs";

                    if (data_count) {
                        //add the imports
                        const auto &imports_template_code = GetDataFactoryImportsTemplate();
                        gen.code.append(imports_template_code);

                        //add the classes
                        const auto &template_code = GetDataFactoryTemplate();
                        for (int i = 0; i < data_count; ++i) {
                            const auto cls = worker_index->data_classes()->Get(i);
                            const auto it = file_id_module_name.find(cls->file_name_id());
                            const auto id = cls->file_name_id();
                            assert(it != file_id_module_name.end());
                            const std::string module_name{it->second};
                            gen.code.append(
                                    mstch::render(template_code, mstch::map{
                                            {"MODULE_NAME", module_name},
                                            {"CLASS_NAME",  cls->class_name()->str()},
                                            {"CLASS_ID",    cls->class_id()}
                                    }));
                        }
                    }

                    if (service_count) {
                        const auto &imports_template_code = GetServiceFactoryImportsTemplate();
                        gen.code.append(imports_template_code);
                        const auto &template_code = GetServiceFactoryTemplate();
                        for (int i = 0; i < service_count; ++i) {
                            const auto cls = worker_index->service_classes()->Get(i);
                            const auto it = file_id_module_name.find(cls->file_name_id());
                            assert(it != file_id_module_name.end());
                            const std::string module_name{it->second};
                            gen.code.append(
                                    mstch::render(template_code, mstch::map{
                                            {"MODULE_NAME", module_name},
                                            {"CLASS_NAME",  cls->class_name()->str()},
                                            {"CLASS_ID",    cls->class_id()}
                                    }));
                        }
                    }

                    assert(!gen.code.empty());

//                    write_whole_binary_file("/home/scott/estate-factory.mjs", gen.code.data(), gen.code.size());

                    return std::move(gen);
                }
            public:
                static void print_constraints(const std::string& label, const LogContext& log_context, const v8::Isolate::CreateParams& create_params) {
                    const auto &c = create_params.constraints;
                    log_trace(log_context, "{} Constraints: max_semi_space_size_in_kb {} code_range_size_in_bytes {} "
                                           "max_old_generation_size_in_bytes {} max_old_space_size {} "
                                           "max_young_generation_size_in_bytes {} max_zone_pool_size {}", label,
                                           c.max_semi_space_size_in_kb(),
                              c.code_range_size_in_bytes(),
                              c.max_old_generation_size_in_bytes(),
                              c.max_old_space_size(),
                              c.max_young_generation_size_in_bytes(),
                              c.max_zone_pool_size());
                }
                static EngineResultCode<EngineU>
                CreateEngine(const LogContext &log_context, Buffer<WorkerIndexProto> worker_index, Buffer<EngineSourceProto> engine_source,
                             size_t max_heap_size) {
                    using Result = EngineResultCode<EngineU>;

                    assert(max_heap_size > 0);

                    const char *FROM = "CreateEngine";

                    //create the isolate, global object, and context
                    v8::Isolate::CreateParams create_params;

                    //TODO: Allow ArrayBuffer allocation but make it limited to a max memory value from config.
                    create_params.array_buffer_allocator = new DisabledArrayBufferAllocator();

                    create_params.constraints = v8::ResourceConstraints{};
                    create_params.constraints.ConfigureDefaultsFromHeapSize(0, max_heap_size);
                    const auto total = create_params.constraints.code_range_size_in_bytes() +
                            create_params.constraints.max_young_generation_size_in_bytes() +
                            create_params.constraints.max_old_generation_size_in_bytes();
                    log_trace(log_context, "Total allocatable memory: {} bytes", total);

                    auto allocator = create_params.array_buffer_allocator;
                    auto isolate = v8::Isolate::New(create_params);
                    v8::HandleScope handle_scope(isolate);
                    auto context = v8::Context::New(isolate, 0, v8::ObjectTemplate::New(isolate));
                    v8::Context::Scope context_scope(context);

                    //////////////////////////////////////////////////////////////////////////////
                    // Create the module cache
                    ModuleCache module_cache{isolate};
                    context->SetEmbedderData(0, v8::External::New(isolate, &module_cache));

                    /////////////////////////////////////////////////////////////////////
                    // Create the estate-server-internal module (that the factory imports)
                    const static std::vector<std::string> server_internal_module_export_strings{
                            std::string{ESTATE_EXISTING_DATA_CTOR_STR},
                            std::string{ESTATE_EXISTING_SERVICE_CTOR_STR}
                    };
                    const static std::string server_internal_module_name_str{ESTATE_KERNEL_INTERNAL_MODULE_NAME};
                    std::vector<v8::Local<v8::String>> server_internal_module_exports{};
                    for (const auto &export_str: server_internal_module_export_strings) {
                        server_internal_module_exports.emplace_back(std::move(V8_STR(export_str)));
                    }
                    auto server_internal_module_name = V8_STR(server_internal_module_name_str);
                    auto server_internal_module = v8::Module::CreateSyntheticModule(
                            isolate,
                            server_internal_module_name,
                            server_internal_module_exports,
                            [](v8::Local<v8::Context> context, v8::Local<v8::Module> module) {
                                auto isolate = context->GetIsolate();
                                //Constructors for existing objects
                                V8_EXPORT_CTOR(module, native::runtime_internal, ESTATE_EXISTING_SERVICE_CTOR, service);
                                V8_EXPORT_CTOR(module, native::runtime_internal, ESTATE_EXISTING_DATA_CTOR, object);
                                return v8::MaybeLocal<v8::Value>(v8::Boolean::New(isolate, true));
                            });
                    //Instanciate and evaluate
                    server_internal_module->InstantiateModule(context, [](v8::Local<v8::Context>, v8::Local<v8::String>, v8::Local<v8::Module>) {
                        assert(false); //shouldn't import anything
                        return v8::MaybeLocal<v8::Module>();
                    }).Check();
                    assert(server_internal_module->GetStatus() == v8::Module::kInstantiated);
                    server_internal_module->Evaluate(context).ToLocalChecked();
                    assert(server_internal_module->GetStatus() == v8::Module::kEvaluated);

                    {
                        const auto worked = module_cache.add_module(true, std::string_view{server_internal_module_name_str}, server_internal_module);
                        assert(worked);
                    }

                    //////////////////////////////////////////////////////////////////////////////////////////
                    // Create the estate-server module (that the user imports directly)
                    const static std::vector<std::string> server_module_export_strings{
                            std::string{ESTATE_NEW_MESSAGE_CTOR_STR},
                            std::string{ESTATE_NEW_DATA_CTOR_STR},
                            std::string{ESTATE_NEW_SERVICE_CTOR_STR},
                            std::string{ESTATE_SYSTEM_OBJECT_NAME},
                            std::string{ESTATE_CREATE_UUID_FUNCTION_STR}
                    };
                    const static std::string server_module_name_str{ESTATE_KERNEL_MODULE_NAME};
                    std::vector<v8::Local<v8::String>> server_module_exports{};
                    for (const auto &export_str: server_module_export_strings) {
                        server_module_exports.emplace_back(std::move(V8_STR(export_str)));
                    }
                    auto server_module_name = V8_STR(server_module_name_str);
                    auto server_module = v8::Module::CreateSyntheticModule(
                            isolate,
                            server_module_name,
                            server_module_exports,
                            [](v8::Local<v8::Context> context, v8::Local<v8::Module> module) {
                                auto isolate = context->GetIsolate();

                                V8_EXPORT_FUNCTION(module, native::runtime, ESTATE_NEW_MESSAGE_CTOR);
                                V8_EXPORT_FUNCTION(module, native::runtime, ESTATE_CREATE_UUID_FUNCTION);
                                V8_EXPORT_CTOR(module, native::runtime, ESTATE_NEW_SERVICE_CTOR, service);
                                V8_EXPORT_CTOR(module, native::runtime, ESTATE_NEW_DATA_CTOR, object);

                                //Server object
                                {
                                    auto server_template = v8::ObjectTemplate::New(isolate);

                                    V8_DEFINE_OBJECT_FUNCTION(server_template, native::runtime::system, ESTATE_GET_SERVICE_FUNCTION);
                                    V8_DEFINE_OBJECT_FUNCTION(server_template, native::runtime::system, ESTATE_REVERT_FUNCTION);
                                    V8_DEFINE_OBJECT_FUNCTION(server_template, native::runtime::system, ESTATE_GET_DATA_FUNCTION);
                                    V8_DEFINE_OBJECT_FUNCTION_N(server_template, native::runtime::system, ESTATE_DELETE_FUNCTION,
                                                                ESTATE_DELETE_FUNCTION_STR);
                                    V8_DEFINE_OBJECT_FUNCTION(server_template, native::runtime::system, ESTATE_SAVE_DATA_GRAPHS_FUNCTION);
                                    V8_DEFINE_OBJECT_FUNCTION(server_template, native::runtime::system, ESTATE_SAVE_DATA_FUNCTION);
                                    V8_DEFINE_OBJECT_FUNCTION(server_template, native::runtime::system, ESTATE_SEND_MESSAGE_FUNCTION);

                                    V8_EXPORT_OBJECT(module, server_template, ESTATE_SYSTEM_OBJECT_NAME);
                                }

                                return v8::MaybeLocal<v8::Value>(v8::Boolean::New(isolate, true));
                            });
                    //Instanciate and evaluate the server module
                    server_module->InstantiateModule(context, [](v8::Local<v8::Context>, v8::Local<v8::String>, v8::Local<v8::Module>) {
                        assert(false); //shouldn't import anything
                        return v8::MaybeLocal<v8::Module>();
                    }).Check();
                    assert(server_module->GetStatus() == v8::Module::kInstantiated);
                    server_module->Evaluate(context).ToLocalChecked();
                    assert(server_module->GetStatus() == v8::Module::kEvaluated);
                    // Add the server module to the module cache, so it can be found
                    {
                        const auto worked = module_cache.add_module(true, std::string_view{server_module_name_str}, server_module);
                        assert(worked);
                    }

                    //////////////////////////////////////////////////////////////////////////////////////////
                    // Disallow eval
                    {
                        static const std::string eval_str{ESTATE_EVAL_FUNCTION_NAME};
                        auto worked = context->Global()->Delete(context, V8_STR(eval_str)).ToChecked();
                        assert(worked);
                    }

                    //////////////////////////////////////////////////////////////////////////////////////////
                    // Setup console in global
                    {
                        static const std::string console_str{ESTATE_CONSOLE_OBJECT_NAME};
                        auto console_value = context->Global()->Get(context, V8_STR(console_str)).ToLocalChecked();
                        assert(console_value->IsObject());
                        auto console = console_value.As<v8::Object>();

                        v8::Local<v8::Function> log_func;
                        v8::Local<v8::Function> error_func;

                        if (worker_index->is_debug()) {
                            log_func = v8::FunctionTemplate::New(isolate, native::console::log)->GetFunction(context).ToLocalChecked();
                            error_func = v8::FunctionTemplate::New(isolate, native::console::error)->GetFunction(context).ToLocalChecked();
                        } else {
                            auto noop_func_template = v8::FunctionTemplate::New(isolate, native::noop);
                            log_func = noop_func_template->GetFunction(context).ToLocalChecked();
                            error_func = noop_func_template->GetFunction(context).ToLocalChecked();
                        }

                        static const std::string log_str{ESTATE_CONSOLE_LOG_FUNCTION_STR};
                        {
                            const auto worked = console->Set(context, V8_STR(log_str), log_func).ToChecked();
                            assert(worked);
                        }
                        static const std::string error_str{ESTATE_CONSOLE_ERROR_FUNCTION_STR};
                        {
                            const auto worked = console->Set(context, V8_STR(error_str), error_func).ToChecked();
                            assert(worked);
                        }
                    }

                    ///////////////////////////////////////////////////////////////////////////////////
                    // Load modules

                    if (!worker_index->file_names() || worker_index->file_names()->size() == 0) {
                        log_error(log_context, "WorkerIndex contained no code to load.");
                        return Result::Error(Code::ScriptEngine_NoCode);
                    }

                    assert(engine_source->code_files() && engine_source->code_files()->size() > 0 &&
                           engine_source->code_files()->size() == worker_index->file_names()->size());

                    // Build the module paths
                    std::unordered_map<std::string_view, std::string_view> file_code{};
                    std::unordered_map<std::string_view, std::string> file_module_name{};
                    for (int i = 0; i < worker_index->file_names()->size(); ++i) {
                        auto f = worker_index->file_names()->Get(i);
                        auto file_name = f->file_name()->string_view();
                        auto file_id = f->file_name_id();
                        if (file_id > engine_source->code_files()->size()) {
                            log_error(log_context, "The file id referred to code that didn't exist");
                            return Result::Error(Code::ScriptEngine_BadFileId);
                        }
                        file_code[file_name] = engine_source->code_files()->Get(file_id - 1)->string_view();
                    }

                    const auto worker_name = worker_index->worker_name()->string_view();
                    //note: the synthetic (C++ backed) modules don't have a source.

                    // Compile all the user modules and load them into the module cache
                    for (const auto[file_name, code]: file_code) {
                        V8_COMPILE_MODULE(module, worker_name, code, file_name);
                        auto worked = module_cache.add_module(false, file_name, module);
                        if (!worked) {
                            const auto error = worked.get_error();
                            V8_THROW_FMT(FROM, "Unrecoverable error loading module {}: {}", file_name, get_code_name(error));
                            return Result::Error(error);
                        }
                    }

                    //Generate, compile, and cache the factory module
                    const auto factory_code = GenerateRuntimeCode(worker_index.get_view(), module_cache);
                    V8_COMPILE_MODULE(factory_module, worker_name, factory_code.code, factory_code.file_name);
                    {
                        const auto worked = module_cache.add_module(true, factory_code.file_name, factory_module);
                        assert(worked);
                    }

                    // Instaciate all the modules that need instanciating directly.
                    for (auto&[_, module_def]: module_cache.get_module_defs()) {
                        auto module = module_def.module.Get(isolate);

                        const auto status = module->GetStatus();

                        if (status >= v8::Module::kInstantiated || module_def.instanciating)
                            continue;

                        module_def.instanciating = true; //to prevent cycles

                        v8::TryCatch try_catch(isolate);
                        const auto unused = module->InstantiateModule(context, &EngineFactory::ResolveCallback);

                        module_def.instanciating = false;

                        if (try_catch.HasCaught()) {
                            auto ex = create_script_exception(log_context, isolate, try_catch);
                            log_script_exception(log_context, ex);
                            log_error(log_context, "Failed to instanciate module {} due to script exception", module_def.file_name);
                            return Result::Error(ex);
                        }

                        if (module->GetStatus() != v8::Module::kInstantiated) {
                            log_error(log_context, "Failed to instanciate module {} for an unknown reason", module_def.file_name);
                            return Result::Error(Code::ScriptEngine_FailedToInstanciateModuleUnknownReason);
                        }
                    }

                    //Evaluate all the modules
                    for (const auto&[_, module_def]: module_cache.get_module_defs()) {
                        auto module = module_def.module.Get(isolate);

                        const auto status = module->GetStatus();
                        if (status == v8::Module::kEvaluated)
                            continue;
                        if (status != v8::Module::kInstantiated) {
                            log_error(log_context, "Failed to evaluate module {}, it had previously failed to completely instanciate.",
                                      module_def.file_name);
                            return Result::Error(Code::ScriptEngine_FailedToEvaluateModuleUnknownReason);
                        }

                        v8::TryCatch try_catch(isolate);
                        auto unused = module->Evaluate(context);
                        if (try_catch.HasCaught()) {
                            auto ex = create_script_exception(log_context, isolate, try_catch);
                            log_script_exception(log_context, ex);
                            log_error(log_context, "Failed to evaluate module {}", module_def.file_name);
                            return Result::Error(ex);
                        }
                        if (module->GetStatus() != v8::Module::kEvaluated) {
                            log_error(log_context, "Failed to evaluate module {} for an unknown reason", module_def.file_name);
                            return Result::Error(Code::ScriptEngine_FailedToEvaluateModuleUnknownReason);
                        }
                    }

                    //remove the module cache
                    context->SetEmbedderData(0, v8::Undefined(isolate));

                    //Get the Object Factory Functions
                    auto object_factory_functions = std::make_unique<ObjectFactoryFunctions>();
                    auto ns = factory_module->GetModuleNamespace()->ToObject(context).ToLocalChecked();
                    if (worker_index->service_classes()) {
                        for (auto i = 0; i < worker_index->service_classes()->size(); ++i) {
                            auto class_id = worker_index->service_classes()->Get(i)->class_id();
                            auto graft_func = v8::Local<v8::Function>::Cast(
                                    ns->Get(context, V8_STR(fmt::format(ESTATE_GRAFT_FUNC_FORMAT,
                                                                        class_id))).ToLocalChecked());
                            object_factory_functions->graft_functions[class_id].Reset(isolate, graft_func);
                            auto new_func = v8::Local<v8::Function>::Cast(
                                    ns->Get(context, V8_STR(fmt::format(ESTATE_NEW_FUNC_FORMAT,
                                                                        class_id))).ToLocalChecked());
                            object_factory_functions->new_functions[class_id].Reset(isolate, new_func);
                        }
                    }
                    if (worker_index->data_classes()) {
                        for (auto i = 0; i < worker_index->data_classes()->size(); ++i) {
                            auto class_id = worker_index->data_classes()->Get(i)->class_id();
                            auto graft_func = v8::Local<v8::Function>::Cast(
                                    ns->Get(context, V8_STR(fmt::format(ESTATE_GRAFT_FUNC_FORMAT,
                                                                        class_id))).ToLocalChecked());
                            object_factory_functions->graft_functions[class_id].Reset(isolate, graft_func);
                        }
                    }

                    return Result::Ok(std::make_unique<Engine>(
                            std::move(worker_index),
                            isolate,
                            allocator,
                            std::move(context),
                            std::move(object_factory_functions)));
                }
            };
            Engine *get_engine(v8::Isolate *isolate_) {
                V8_SCOPE(isolate_);
                v8::Handle<v8::External> __embedder_data = v8::Handle<v8::External>::Cast(context->GetEmbedderData(0));
                return static_cast<Engine *>(__embedder_data->Value());
            }
            template<data::ObjectType OT>
            EngineResultCode<v8::Local<v8::Object>> js_load_object(v8::Isolate *isolate_, const data::ObjectReferenceS &ref, bool rethrow) {
                using Result = EngineResultCode<v8::Local<v8::Object>>;
                static const std::string OBJECT_TYPE = OT == data::ObjectType::WORKER_OBJECT ? "Data" : "Service";

                /* Data and Services load differently.
                 * Data:
                 *  If it doesn't exist, an error is returned.
                 * Service:
                 *  If it doesn't exist, it's created.
                 * */

                V8_ESCAPABLE_SCOPE(isolate_);

                auto engine = get_engine(isolate);
                auto call_context = engine->get_call_context();
                auto working_set = call_context->get_working_set();

                // Pre-load the object to see if it exists so we know what js function to call to wrap its instance in.
                // This is A-OK (speed-wise) because it's cached in the working set.
                UNWRAP_OR_RETURN(maybe_object, working_set->maybe_resolve(ref, false));

                v8::Local<v8::Function> func;
                if (maybe_object.has_value()) {
                    func = engine->get_graft_function(ref->class_id).Get(isolate);
                } else if (OT == data::ObjectType::WORKER_SERVICE) {
                    //Only create it automatically if it's a Service.
                    func = engine->get_new_function(ref->class_id).Get(isolate);
                } else {
                    return Result::Error(Code::Datastore_ObjectNotFound);
                }

                const auto primary_key_str = ref->get_primary_key().view();
                v8::Local<v8::Value> primary_key = V8_STRING(primary_key_str.data(), primary_key_str.size());

                v8::TryCatch try_catch{isolate};
                v8::Local<v8::Value> result_val;
                engine->clear_internal_error();
                if (!func->Call(context, v8::Null(isolate), 1, &primary_key).ToLocal(&result_val)) {
                    if (engine->get_internal_error().has_value()) {
                        return Result::Error(engine->get_internal_error().value());
                    }

                    const auto &log_context = call_context->get_log_context();
                    if (try_catch.HasCaught()) {
                        auto ex = create_script_exception(log_context, isolate, try_catch);
                        log_script_exception(log_context, ex);
                        log_warn(log_context, "Failed to load {} due to a script exception", OBJECT_TYPE);
                        if (rethrow) {
                            try_catch.ReThrow();
                        }
                        return Result::Error(std::move(ex));
                    }

                    assert(false);
                    log_error(log_context, "Unable to apply service for an unknown reason");
                    return Result::Error(Code::ScriptEngine_UnknownFunctionCallFailure);
                }

                assert(result_val->IsObject());
                auto result = v8::Local<v8::Object>::Cast(result_val);
                return Result::Ok(handle_scope.Escape(result));
            }
            [[maybe_unused]] int get_array_length(v8::Isolate *isolate, v8::Local<v8::Array> array) {
                auto context = isolate->GetCurrentContext();
                static const std::string LENGTH_NAME{"length"};
                auto length_name_str = v8::String::NewFromUtf8(isolate, LENGTH_NAME.data(), v8::NewStringType::kNormal,
                                                               LENGTH_NAME.length()).ToLocalChecked();
                return array->Get(context, length_name_str).ToLocalChecked()->Uint32Value(context).ToChecked();
            }
            [[maybe_unused]] void print_value_details(v8::Local<v8::Value> local_value) {

                auto value = *local_value;

                std::cout << "undefined: " << value->IsUndefined() << std::endl;
                std::cout << "null: " << value->IsNull() << std::endl;
                std::cout << "true: " << value->IsTrue() << std::endl;
                std::cout << "false: " << value->IsFalse() << std::endl;
                std::cout << "name: " << value->IsName() << std::endl;
                std::cout << "string: " << value->IsString() << std::endl;
                std::cout << "symbol: " << value->IsSymbol() << std::endl;
                std::cout << "function: " << value->IsFunction() << std::endl;
                std::cout << "array: " << value->IsArray() << std::endl;
                std::cout << "object: " << value->IsObject() << std::endl;
                std::cout << "boolean: " << value->IsBoolean() << std::endl;
                std::cout << "number: " << value->IsNumber() << std::endl;
                std::cout << "external: " << value->IsExternal() << std::endl;
                std::cout << "isint32: " << value->IsInt32() << std::endl;
                std::cout << "isuint32: " << value->IsUint32() << std::endl;
                std::cout << "date: " << value->IsDate() << std::endl;
                std::cout << "argument object: " << value->IsArgumentsObject() << std::endl;
                std::cout << "boolean object: " << value->IsBooleanObject() << std::endl;
                std::cout << "number object: " << value->IsNumberObject() << std::endl;
                std::cout << "string object: " << value->IsStringObject() << std::endl;
                std::cout << "symbol object: " << value->IsSymbolObject() << std::endl;
                std::cout << "native error: " << value->IsNativeError() << std::endl;
                std::cout << "regexp: " << value->IsRegExp() << std::endl;
                std::cout << "generator function: " << value->IsGeneratorFunction() << std::endl;
                std::cout << "generator object: " << value->IsGeneratorObject() << std::endl;
            }
            void set_stack_limit(v8::Isolate *isolate) {
                //See https://fw.hardijzer.nl/?p=97 for rationale
                //See https://chromium.googlesource.com/chromium/blink/+/master/Source/bindings/core/v8/V8Initializer.cpp#483 for example in Chromium
                uint32_t here;
                isolate->SetStackLimit(reinterpret_cast<uintptr_t>(&here - ESTATE_JAVASCRIPT_ISOLATE_MAX_STACKSIZE / sizeof(uint32_t *)));
            }
            void log_script_exception(const LogContext &log_context, const ScriptException &ex) {
                log_info(log_context, "Script exception caught. Message: {}, Stack Trace: {}",
                         ex.message.value_or("<empty>"),
                         ex.stack_trace.value_or("<empty>"));
            }
            std::string value_to_string(v8::Isolate *isolate, const v8::Local<v8::Value> &value) {
                v8::String::Utf8Value utf8_value(isolate, value);
                return std::string(*utf8_value);
            }
            ScriptException create_script_exception(const LogContext &log_context, v8::Isolate *isolate_, const v8::TryCatch &try_catch) {
                V8_SCOPE(isolate_);
                assert(try_catch.HasCaught());

                //TODO: add custom exception string formatting

                //SJ NOTES: The reason I'm not including the name of the exception is because it's not available without doing a string parse (which would be slow).
                // The stack value (if it exists) must have a specific string value that the browser expects. That's the reason I'm sending back info that looks
                // like it's a duplicate. In addition, sometimes the message will exist but the stack won't and (maybe) vice-versa though I've never seen that
                // happen.

                std::optional<std::string> maybe_message{};
                const auto ex_value = try_catch.Exception();
                if (!ex_value.IsEmpty()) {
                    maybe_message.emplace(value_to_string(isolate, ex_value));
                }

                std::optional<std::string> stack_trace{};
                v8::Local<v8::Value> stack_trace_value;
                if (try_catch.StackTrace(context).ToLocal(&stack_trace_value)) {
                    stack_trace.emplace(value_to_string(isolate, stack_trace_value));
                }

                if (!maybe_message.has_value() && !stack_trace.has_value()) {
                    log_warn(log_context, "Created an empty script exception because .stack and .message were empty.");
                }

                return ScriptException{
                        std::move(maybe_message),
                        std::move(stack_trace)
                };
            }
            class JsObjectRuntime : public virtual IObjectRuntime {
                using EnginePool = Pool<Engine, EngineError>;
                using EnginePoolS = PoolS<Engine, EngineError>;
                std::mutex _worker_id_engine_pool_mutex{};
                std::unordered_map<WorkerId, EnginePoolS> _worker_id_engine_pool{};
                size_t _max_heap_size;
            private:
                EngineResultCode<Buffer<WorkerProcessUserResponseProto>>
                make_call_service_method_response(v8::Isolate *isolate_,
                                                       const CallContextS &call_context,
                                                       const v8::Local<v8::Value> &return_value,
                                                       bool &data_saved) {
                    using Result = EngineResultCode<Buffer<WorkerProcessUserResponseProto>>;
                    data_saved = false;

                    V8_SCOPE(isolate_);
                    const auto &log_context = call_context->get_log_context();
                    auto working_set = call_context->get_working_set();

                    // Flush all the working set's objects to ensure there's no unsaved changes
                    for (const auto &[_, object]: working_set->get_cached_objects()) {
                        if (!object->is_data())
                            continue;
                        if (object->is_deleted() && object->is_permanent_deleted())
                            continue; //if both states are deleted we don't care about any outstanding changes
                        UNWRAP_OR_RETURN(unsaved, object->flush(std::nullopt));
                        if (unsaved) {
                            log_warn(log_context, "Object {} pk {} contained unsaved changes", object->get_reference()->class_id,
                                     object->get_reference()->get_primary_key().view());
                            return Result::Error(Code::Datastore_UnsavedChanges);
                        }
                    }

                    fbs::Builder outer_builder{};

                    // Get the generated deltas
                    fbs::Offset<fbs::Vector<fbs::Offset<DataDeltaBytesProto>>> delta_bytes_off{};
                    auto maybe_deltas = call_context->extract_deltas();
                    if (maybe_deltas.has_value()) {
                        std::vector<fbs::Offset<DataDeltaBytesProto>> delta_bytes_vec{};
                        const auto deltas = std::move(maybe_deltas.value());
                        for (const auto &delta: deltas) {
                            auto vec = outer_builder.CreateVector(delta.as_u8(), delta.size());
                            delta_bytes_vec.push_back(CreateDataDeltaBytesProto(outer_builder, vec));
                        }
                        data_saved = !delta_bytes_vec.empty();
                        delta_bytes_off = outer_builder.CreateVector(delta_bytes_vec);
                    }

                    // Get the fired events
                    fbs::Offset<fbs::Vector<fbs::Offset<MessageBytesProto>>> event_bytes_off{};
                    auto maybe_events = call_context->extract_fired_events();
                    if (maybe_events) {
                        std::vector<fbs::Offset<MessageBytesProto>> event_bytes_vec{};
                        const auto events = std::move(maybe_events.value());
                        for (const auto &event: events) {
                            auto vec = outer_builder.CreateVector(event.as_u8(), event.size());
                            event_bytes_vec.push_back(CreateMessageBytesProto(outer_builder, vec));
                        }
                        event_bytes_off = outer_builder.CreateVector(event_bytes_vec);
                    }

                    auto buffer_pool = call_context->get_buffer_pool();

                    // Get the console log
                    auto console_log = call_context->get_console_log();
                    fbs::Offset<fbs::Vector<u8>> console_log_vec_off = 0;
                    auto maybe_console_log_buffer = create_console_log_proto(buffer_pool, console_log);
                    if (maybe_console_log_buffer.has_value()) {
                        auto console_log_buffer = std::move(maybe_console_log_buffer.value());
                        console_log_vec_off = outer_builder.CreateVector(console_log_buffer.as_u8(), console_log_buffer.size());
                    }

                    // Serialize the method return value
                    fbs::Builder inner_builder{};
                    ValueUnionProto unused;
                    auto result_off_r = serialize(log_context, isolate, inner_builder, return_value, unused, std::nullopt);
                    if (!result_off_r)
                        return Result::Error(result_off_r.get_error());
                    inner_builder.Finish(
                            CreateUserResponseUnionWrapperProto(inner_builder, UserResponseUnionProto::CallServiceMethodResponseProto,
                                                                CreateCallServiceMethodResponseProto(inner_builder,
                                                                                                         result_off_r.unwrap()).Union()));

                    const auto response_proto = CreateWorkerProcessUserResponseProto(
                            outer_builder,
                            outer_builder.CreateVector(inner_builder.GetBufferPointer(), inner_builder.GetSize()),
                            delta_bytes_off,
                            event_bytes_off,
                            console_log_vec_off
                    );

                    return Result::Ok(finish_and_copy_to_buffer(outer_builder, buffer_pool, response_proto));
                }
                EnginePoolS get_engine_pool(const WorkerId worker_id) {
                    std::lock_guard<std::mutex> lck(_worker_id_engine_pool_mutex);
                    auto it = _worker_id_engine_pool.find(worker_id);
                    if (it != _worker_id_engine_pool.end()) {
                        auto pool = it->second;
                        return std::move(pool);
                    }
                    auto engine_pool = std::make_shared<EnginePool>();
                    _worker_id_engine_pool[worker_id] = engine_pool;
                    return std::move(engine_pool);
                }
                EngineResultCode<EngineHandle> get_engine(CallContextS call_context) {
                    using OutResult = EngineResultCode<EngineHandle>;

                    auto txn = call_context->get_transaction();
                    auto log_context = call_context->get_log_context();

                    const auto worker_id = txn->get_worker_id();
                    const auto worker_version = txn->get_worker_version();

                    auto engine_pool = get_engine_pool(worker_id);

                    // Get the Engine
                    auto engine_handle_r = engine_pool->get_resource([this, &log_context, &txn]() {
                        using Result = EngineResultCode<EngineU>;

                        // Get the EngineSource
                        auto engine_source_r = txn->get_engine_source();
                        if (!engine_source_r)
                            return Result::Error(engine_source_r.get_error());
                        auto engine_source = engine_source_r.unwrap();

                        // Get the WorkerIndex
                        auto worker_index_r = txn->get_worker_index();
                        if (!worker_index_r)
                            return Result::Error(worker_index_r.get_error());
                        auto worker_index = worker_index_r.unwrap();

                        // Create the Engine
                        auto engine_r = EngineFactory::CreateEngine(log_context,
                                                                    std::move(worker_index),
                                                                    std::move(engine_source),
                                                                    this->_max_heap_size);
                        if (!engine_r)
                            return Result::Error(engine_r.get_error());

                        return Result::Ok(std::move(engine_r.unwrap()));
                    }, [&worker_version](EngineU &engine) {
                        return engine->worker_version() == worker_version;
                    });

                    if (!engine_handle_r) {
                        return OutResult::Error(engine_handle_r.get_error());
                    }

                    auto engine_handle = engine_handle_r.unwrap();
                    assert(engine_handle.get()->worker_version() == worker_version);

                    //set the current CallContext (weakly) so in-engine Js functions can get it
                    engine_handle.get()->set_call_context(call_context);

                    return OutResult::Ok(std::move(engine_handle));
                }
                ResultCode<std::pair<v8::Local<v8::String>, std::string_view>> get_service_method_name(const LogContext &log_context,
                                                                                                       v8::Isolate *isolate_,
                                                                                                       const WorkerIndexProto &worker_index,
                                                                                                       ClassId class_id,
                                                                                                       MethodId method_id) {
                    using Result = ResultCode<std::pair<v8::Local<v8::String>, std::string_view>>;

                    V8_ESCAPABLE_SCOPE(isolate_);

                    if (!worker_index.service_classes())
                        return Result::Error(Code::ScriptEngine_ClassNotFound);

                    for (int i = 0; i < worker_index.service_classes()->size(); ++i) {
                        auto cls = worker_index.service_classes()->Get(i);
                        if (cls->class_id() != class_id)
                            continue;
                        if (!cls->methods()) {
                            log_error(log_context, "Unable to get the service method name because the specified class {} had no methods", class_id);
                            return Result::Error(Code::ScriptEngine_MethodNotFound);
                        }
                        for (int j = 0; j < cls->methods()->size(); ++j) {
                            auto method = cls->methods()->Get(j);
                            if (method->method_id() == method_id) {
                                auto method_name_view = method->method_name()->string_view();
                                auto method_name = V8_STRING(method_name_view.data(), method_name_view.size());
                                return Result::Ok(std::make_pair(handle_scope.Escape(method_name), method_name_view));
                            }
                        }
                        log_error(log_context, "Unable to get the service method name because the method {} wasn't found on the class {}", method_id,
                                  class_id);
                        return Result::Error(Code::ScriptEngine_MethodNotFound);
                    }

                    log_error(log_context, "Unable to get the service method name because the class {} wasn't found", class_id);
                    return Result::Error(Code::ScriptEngine_ClassNotFound);
                }
                ResultCode<v8::Local<v8::Function>>
                get_service_method(const LogContext &log_context, v8::Isolate *isolate_, v8::Local<v8::Object> &service,
                                   v8::Local<v8::String> &method_name) {
                    using Result = ResultCode<v8::Local<v8::Function>>;

                    V8_ESCAPABLE_SCOPE(isolate_);

                    v8::Local<v8::Object> service_proto = v8::Local<v8::Object>::Cast(service->GetPrototype());

                    v8::Local<v8::Value> method_val;
                    if (!service_proto->Get(context, method_name).ToLocal(&method_val)) {
                        log_error(log_context, "Unable to get the service method off the service prototype");
                        return Result::Error(Code::ScriptEngine_UnableToGetMethod);
                    }

                    if (!method_val->IsFunction()) {
                        log_error(log_context, "Unable to get the service method because it wasn't a function");
                        return Result::Error(Code::ScriptEngine_UnableToGetMethodBecauseItWasNotAFunction);
                    }

                    const auto method = v8::Local<v8::Function>::Cast(method_val);

                    return Result::Ok(handle_scope.Escape(method));
                }
            public:
                EngineResultCode<CallServiceMethodResult>
                call_service_method(CallContextS call_context, const CallServiceMethodRequestProto *request) override {
                    using Result = EngineResultCode<CallServiceMethodResult>;
                    const auto &log_context = call_context->get_log_context();
                    Stopwatch timing_overall{log_context};

                    Stopwatch timing_delta_application{log_context};
                    auto txn = call_context->get_transaction();
                    auto working_set = call_context->get_working_set();

                    // Apply in-memory changes to any Data the arguments reference
                    if (request->referenced_data_deltas() && request->referenced_data_deltas()->size()) {
                        for (auto i = 0; i < request->referenced_data_deltas()->size(); ++i) {
                            const auto *delta = request->referenced_data_deltas()->Get(i);
                            WORKED_OR_RETURN(data::apply_inbound_delta(*call_context->get_reusable_builder(true),
                                                                       *delta,
                                                                       working_set,
                                                                       call_context->get_buffer_pool(),
                                                                       false));
                        }
                    }
                    timing_delta_application.log_elapsed("Delta Application");

                    // Get the engine
                    Stopwatch timing_get_engine{log_context};
                    UNWRAP_OR_RETURN(engine, get_engine(call_context));
                    timing_get_engine.log_elapsed("Get engine");

                    Stopwatch setup_isolate_context{log_context};
                    V8_SCOPE_INHERIT_CONTEXT(engine.get()->isolate(), engine.get()->parent_context());
                    set_stack_limit(isolate);
                    setup_isolate_context.log_elapsed("Setup isolate and context");

                    Stopwatch timing_js_service_object{log_context};

                    // Set the isolate so WorkingSet/Object/Property etc. can access it.
                    call_context->set_isolate(isolate);

                    const auto class_id = request->class_id();
                    const auto &worker_index = engine.get()->get_worker_index();
                    const auto method_id = request->method_id();

                    auto method_name_pair_r = get_service_method_name(log_context, isolate, worker_index, class_id, method_id);
                    if (!method_name_pair_r)
                        return Result::Error(method_name_pair_r.get_error());
                    auto[method_name, method_name_view] = method_name_pair_r.unwrap();

                    // Get the service object and its prototype so we can call the method on it
                    UNWRAP_OR_RETURN(service, js_load_object<data::ObjectType::WORKER_SERVICE>(isolate,
                                                                                             make_object_reference(data::ObjectType::WORKER_SERVICE,
                                                                                                                   class_id, PrimaryKey{
                                                                                                             request->primary_key()->string_view()}),
                                                                                             false));

                    timing_js_service_object.log_elapsed("Load Service Object");

                    UNWRAP_OR_RETURN(method, get_service_method(log_context, isolate, service, method_name));

                    Stopwatch timing_deserialize_arguments{log_context};

                    // Deserialize the arguments
                    const auto arg_count = request->arguments() ? request->arguments()->size() : 0;
                    v8::Local<v8::Value> arguments[arg_count];
                    for (auto i = 0; i < arg_count; ++i) {
                        const auto value_proto = request->arguments()->Get(i);
                        if (!value_proto) {
                            log_error(log_context, "unable to read argument at index {0} because it was empty", i);
                            return Result::Error(Code::Validator_InvalidRequest);
                        }
                        UNWRAP_OR_RETURN(value, deserialize(log_context, txn, isolate, value_proto));
                        arguments[i] = value;
                    }

                    timing_deserialize_arguments.log_elapsed("Deserialize Arguments");

                    Stopwatch timing_call{log_context};

                    // Call the service method and get the result
                    v8::Local<v8::Value> result_val;
                    engine.get()->clear_internal_error();
                    v8::TryCatch try_catch(isolate);
                    if (!method->Call(context, service, arg_count, arguments).ToLocal(&result_val)) {
                        auto error_code = engine.get()->get_internal_error();
                        if (error_code.has_value()) {
                            return Result::Error(error_code.value()); //already logged
                        }
                        if (try_catch.HasCaught()) {
                            auto ex = create_script_exception(log_context, isolate, try_catch);
                            log_script_exception(log_context, ex);
                            log_warn(log_context, "Failed to call service method due to a script exception");
                            return Result::Error(std::move(ex));
                        }
                        assert(false);
                        log_error(log_context, "Unable to call the service method {0} for an unknown reason", method_name_view);
                        return Result::Error(Code::ScriptEngine_UnableToCallMethod);
                    }
                    assert(!try_catch.HasCaught());

                    timing_call.log_elapsed("Call");

                    Stopwatch timing_save_services{log_context};

                    // Save changes to services automatically.
                    bool services_saved{false};
                    for (auto&[_, object]: working_set->get_cached_objects()) {
                        if (!object->get_reference()->is_service())
                            continue;
                        UNWRAP_OR_RETURN(changed, object->flush_and_save(std::nullopt));
                        if (changed)
                            services_saved = true;
                    }

                    timing_save_services.log_elapsed("Save Services");

                    Stopwatch timing_make_response{log_context};
                    bool data_saved{false};
                    UNWRAP_OR_RETURN(response_buffer, make_call_service_method_response(isolate, call_context, result_val, data_saved));
                    timing_make_response.log_elapsed("Make Response");

                    timing_overall.log_elapsed("Overall");

                    return Result::Ok(CallServiceMethodResult{
                            services_saved || data_saved,
                            std::move(response_buffer)
                    });
                }
                JsObjectRuntime(size_t max_heap_size) :
                        _max_heap_size{max_heap_size} {}
            };
            IObjectRuntimeS create_object_runtime(size_t max_heap_size) {
                return std::make_shared<JsObjectRuntime>(max_heap_size);
            }
            class JsSetupRuntime : public virtual ISetupRuntime {
            public:
                UnitEngineResultCode
                setup(const LogContext &log_context, storage::ITransactionS txn, const SetupWorkerRequestProto *request, bool is_new) override {
                    using Result = UnitEngineResultCode;

                    if (!is_new) {
                        /*NOTE: I'm not sure deletes are strictly needed but I like the idea of locking their fields early in the transaction.*/
                        //Delete the previous worker index
                        WORKED_OR_RETURN(txn->delete_worker_index());
                        log_trace(log_context, "Deleted the previous worker index for worker version {}", request->previous_worker_version());
                        //Delete the previous engine source
                        WORKED_OR_RETURN(txn->delete_engine_source());
                        log_trace(log_context, "Deleted the previous engine source for worker version {}", request->previous_worker_version());
                    }

                    flatbuffers::FlatBufferBuilder builder{};
                    std::vector<fbs::Offset<fbs::String>> source{};
                    const auto worker_index = request->worker_index_nested_root();
                    source.resize(worker_index->file_names()->size());
                    for (int i = 0; i < worker_index->file_names()->size(); ++i) {
                        const auto file_idx = worker_index->file_names()->Get(i)->file_name_id() - 1;
                        source[file_idx] = builder.CreateString(request->worker_code()->Get(file_idx)->string_view());
                    }

                    builder.Finish(CreateEngineSourceProto(builder, builder.CreateVector(source)));

                    //Save the engine source
                    WORKED_OR_RETURN(txn->save_engine_source(BufferView<EngineSourceProto>{builder}));
                    log_trace(log_context, "Saved the engine source for worker version {}", request->worker_version());

                    //Save the worker index
                    WORKED_OR_RETURN(txn->save_worker_index(request->worker_version(),
                                                          BufferView<WorkerIndexProto>{request->worker_index()->data(), request->worker_index()->size()}));
                    log_trace(log_context, "Saved the new worker index for worker version {}", request->worker_version());

                    return Result::Ok();
                }
            };
            ISetupRuntimeS create_setup_runtime() {
                return std::make_shared<JsSetupRuntime>();
            }
            static std::unique_ptr<v8::Platform> platform{};
            bool is_initialized() {
                return platform != nullptr;
            }
            void initialize(const JavascriptConfig &config) {
                v8::V8::InitializeICUDefaultLocation(config.icu_location.c_str());
                v8::V8::InitializeExternalStartupData(config.external_startup_data.c_str());
                platform = v8::platform::NewDefaultPlatform();
                v8::V8::InitializePlatform(platform.get());
                v8::V8::Initialize();
            }
            void shutdown() {
                assert(platform);
                v8::V8::ShutdownPlatform();
                platform = nullptr;
            }
            Result<ValueUnionProto> get_type(const LogContext &log_context, const v8::Local<v8::Value> &value) {
                using Result = Result<ValueUnionProto>;

                if (value->IsUndefined()) {
                    return Result::Ok(ValueUnionProto::UndefinedValueProto);
                }

                if (value->IsNull()) {
                    return Result::Ok(ValueUnionProto::NullValueProto);
                }

                if (value->IsBoolean() || value->IsBooleanObject()) {
                    return Result::Ok(ValueUnionProto::BooleanValueProto);
                }

                if (value->IsNumber() || value->IsNumberObject()) {
                    return Result::Ok(ValueUnionProto::NumberValueProto);
                }

                if (value->IsName() || value->IsString() || value->IsStringObject()) {
                    return Result::Ok(ValueUnionProto::StringValueProto);
                }

                if (value->IsArray()) {
                    return Result::Ok(ValueUnionProto::ArrayValueProto);
                }

                if (value->IsMap()) {
                    return Result::Ok(ValueUnionProto::MapValueProto);
                }

                if (value->IsSet()) {
                    return Result::Ok(ValueUnionProto::SetValueProto);
                }

                if (value->IsDate()) {
                    return Result::Ok(ValueUnionProto::DateValueProto);
                }

                if (value->IsObject()) {
                    auto obj = v8::Local<v8::Object>::Cast(value);
                    if (obj->InternalFieldCount() > 0) {
                        data::Object *target = V8_UNWRAP_OBJECT(obj);
                        const auto object_type = target->get_reference()->type;
                        switch (object_type) {
                            case data::ObjectType::WORKER_SERVICE:
                                return Result::Ok(ValueUnionProto::ServiceReferenceValueProto);
                            case data::ObjectType::WORKER_OBJECT:
                                return Result::Ok(ValueUnionProto::DataReferenceValueProto);
                            default: {
                                log_critical(log_context, "Invalid ObjectType {}", (u8) object_type);
                                assert(false);
                                return Result::Error();
                            }
                        }
                    } else {
                        return Result::Ok(ValueUnionProto::ObjectValueProto);
                    }
                }

                return Result::Error();
            }
            ResultCode<fbs::Offset<ValueProto>>
            serialize(const LogContext &log_context, v8::Isolate *isolate_, flatbuffers::FlatBufferBuilder &builder,
                      const v8::Local<v8::Value> value, ValueUnionProto &type,
                      std::optional<data::TrackerS> maybe_tracker) {
                using Result = ResultCode<fbs::Offset<ValueProto>>;
                V8_ESCAPABLE_SCOPE(isolate_);

                auto type_r = get_type(log_context, value);
                if (!type_r) {
                    log_error(log_context, "Unknown JavaScript value type found during serialization");
                    return Result::Error(Code::ScriptEngine_UnsupportedValueType);
                }

                type = type_r.unwrap();
                switch (type) {
                    case ValueUnionProto::UndefinedValueProto: {
                        return Result::Ok(CreateValueProto(builder, ValueUnionProto::UndefinedValueProto));
                    }
                    case ValueUnionProto::NullValueProto: {
                        return Result::Ok(CreateValueProto(builder, ValueUnionProto::NullValueProto));
                    }
                    case ValueUnionProto::BooleanValueProto: {
                        const auto boolean = v8::Local<v8::Boolean>::Cast(value);
                        return Result::Ok(CreateValueProto(builder, ValueUnionProto::BooleanValueProto,
                                                           CreateBooleanValueProto(builder, boolean->Value()).Union()));
                    }
                    case ValueUnionProto::StringValueProto: {
                        const auto str = builder.CreateString(value_to_string(isolate, value));
                        return Result::Ok(CreateValueProto(builder, ValueUnionProto::StringValueProto, CreateStringValueProto(builder, str).Union()));
                    }
                    case ValueUnionProto::NumberValueProto: {
                        const auto num = v8::Local<v8::Number>::Cast(value);
                        return Result::Ok(
                                CreateValueProto(builder, ValueUnionProto::NumberValueProto, CreateNumberValueProto(builder, num->Value()).Union()));
                    }
                    case ValueUnionProto::ObjectValueProto: {
                        const auto object = v8::Local<v8::Object>::Cast(value);

                        auto names = object->GetOwnPropertyNames(context).ToLocalChecked();
                        const auto names_len = names->Length();
                        if (names_len > 0) {
                            std::vector<fbs::Offset<PropertyProto>> properties{};
                            for (uint32_t i = 0; i < names_len; ++i) {
                                auto n = names->Get(context, i).ToLocalChecked();
                                auto v = object->Get(context, n).ToLocalChecked();
                                ValueUnionProto unused;
                                auto object_value_r = serialize(log_context, isolate, builder, v, unused, maybe_tracker);
                                if (!object_value_r)
                                    return Result::Error(object_value_r.get_error());
                                auto object_value = object_value_r.unwrap();
                                auto name_str = value_to_string(isolate, n);
                                auto name = builder.CreateString(name_str.data(), name_str.size());
                                properties.push_back(CreatePropertyProto(builder, name, object_value));
                            }

                            auto object_value_proto = CreateObjectValueProtoDirect(builder, &properties).Union();
                            auto value_proto = CreateValueProto(builder, ValueUnionProto::ObjectValueProto, object_value_proto);
                            return Result::Ok(value_proto);
                        }

                        return Result::Ok(
                                CreateValueProto(builder, ValueUnionProto::ObjectValueProto, CreateObjectValueProtoDirect(builder, nullptr).Union()));
                    }
                    case ValueUnionProto::ArrayValueProto: {
                        const auto array = v8::Local<v8::Array>::Cast(value);

                        const auto len = array->Length();
                        if (len > 0) {
                            std::vector<fbs::Offset<ArrayItemValueProto>> items{};
                            for (uint32_t i = 0; i < len; ++i) {
                                auto item = array->Get(context, i).ToLocalChecked();
                                ValueUnionProto unused;
                                auto value_r = serialize(log_context, isolate, builder, item, unused, maybe_tracker);
                                if (!value_r)
                                    return Result::Error(value_r.get_error());
                                items.push_back(CreateArrayItemValueProto(builder, i, std::move(value_r.unwrap())));
                            }
                            return Result::Ok(CreateValueProto(builder, ValueUnionProto::ArrayValueProto,
                                                               CreateArrayValueProtoDirect(builder, &items).Union()));
                        }

                        return Result::Ok(CreateValueProto(builder, ValueUnionProto::ArrayValueProto,
                                                           CreateArrayValueProtoDirect(builder, nullptr).Union()));
                    }
                    case ValueUnionProto::DataReferenceValueProto: {
                        const auto obj_val = v8::Local<v8::Object>::Cast(value);
                        data::Object *target = V8_UNWRAP_OBJECT(obj_val);
                        auto handle = target->get_handle();
                        if (maybe_tracker.has_value()) {
                            maybe_tracker.value()->track(handle);
                        }
                        auto ref = handle->get_reference();
                        const auto &primary_key_view = ref->get_primary_key().view();
                        auto primary_key_off = builder.CreateString(primary_key_view.data(), primary_key_view.length());
                        auto off = CreateDataReferenceValueProto(builder, ref->class_id, primary_key_off).Union();
                        return Result::Ok(CreateValueProto(builder, ValueUnionProto::DataReferenceValueProto, off));
                    }
                    case ValueUnionProto::ServiceReferenceValueProto: {
                        const auto obj_val = v8::Local<v8::Object>::Cast(value);
                        data::Object *target = V8_UNWRAP_OBJECT(obj_val);
                        auto ref = target->get_reference();
                        const auto &primary_key_view = ref->get_primary_key().view();
                        auto primary_key_off = builder.CreateString(primary_key_view.data(), primary_key_view.length());
                        auto off = CreateServiceReferenceValueProto(builder, ref->class_id, primary_key_off).Union();
                        return Result::Ok(CreateValueProto(builder, ValueUnionProto::ServiceReferenceValueProto, off));
                    }
                    case ValueUnionProto::MapValueProto: {
                        const auto map = v8::Local<v8::Map>::Cast(value);
                        const auto map_array = map->AsArray();

                        const auto len = map_array->Length();
                        if (len > 0) {
                            std::vector<fbs::Offset<MapValueItemProto>> items{};
                            for (auto i = 0; i < len; i += 2) {
                                const auto key_value = map_array->Get(context, i).ToLocalChecked();
                                ValueUnionProto unused;
                                auto key_r = serialize(log_context, isolate, builder, key_value, unused, maybe_tracker);
                                if (!key_r)
                                    return Result::Error(key_r.get_error());
                                const auto value_value = map_array->Get(context, i + 1).ToLocalChecked();
                                auto value_r = serialize(log_context, isolate, builder, value_value, unused, maybe_tracker);
                                if (!value_r)
                                    return Result::Error(value_r.get_error());
                                items.push_back(CreateMapValueItemProto(builder, key_r.unwrap(), value_r.unwrap()));
                            }

                            return Result::Ok(CreateValueProto(builder, ValueUnionProto::MapValueProto,
                                                               CreateMapValueProtoDirect(builder, &items).Union()));
                        }

                        return Result::Ok(CreateValueProto(builder, ValueUnionProto::MapValueProto,
                                                           CreateMapValueProtoDirect(builder, nullptr).Union()));
                    }
                    case ValueUnionProto::SetValueProto: {
                        const auto set = v8::Local<v8::Set>::Cast(value);
                        const auto set_array = set->AsArray();
                        const auto len = set_array->Length();
                        if (len > 0) {
                            std::vector<fbs::Offset<ValueProto>> items{};
                            for (auto i = 0; i < len; ++i) {
                                const auto set_value = set_array->Get(context, i).ToLocalChecked();
                                ValueUnionProto unused;
                                auto value_r = serialize(log_context, isolate, builder, set_value, unused, maybe_tracker);
                                if (!value_r)
                                    return Result::Error(value_r.get_error());
                                items.push_back(value_r.unwrap());
                            }
                            return Result::Ok(CreateValueProto(builder, ValueUnionProto::SetValueProto,
                                                               CreateSetValueProtoDirect(builder, &items).Union()));
                        }
                        return Result::Ok(CreateValueProto(builder, ValueUnionProto::SetValueProto,
                                                           CreateSetValueProtoDirect(builder, nullptr).Union()));
                    }
                    case ValueUnionProto::DateValueProto: {
                        const auto date = v8::Local<v8::Date>::Cast(value);
                        return Result::Ok(
                                CreateValueProto(builder, ValueUnionProto::DateValueProto, CreateDateValueProto(builder, date->ValueOf()).Union()));
                    }
                    default: {
                        log_critical(log_context, "Unknown type when trying to serialize value");
                        assert(false);
                        return Result::Error(Code::ScriptEngine_UnsupportedValueType);
                    }
                }
            }
            EngineResultCode<v8::Local<v8::Value>>
            deserialize(const LogContext &log_context, storage::ITransactionS txn, v8::Isolate *isolate_, const ValueProto *proto) {
                using Result = EngineResultCode<v8::Local<v8::Value>>;

                V8_ESCAPABLE_SCOPE(isolate_);

                switch (proto->value_type()) {
                    case ValueUnionProto::UndefinedValueProto: {
                        return Result::Ok(handle_scope.Escape(v8::Undefined(isolate)));
                    }
                    case ValueUnionProto::NullValueProto: {
                        return Result::Ok(handle_scope.Escape(v8::Null(isolate)));
                    }
                    case ValueUnionProto::BooleanValueProto: {
                        return Result::Ok(handle_scope.Escape(v8::Boolean::New(isolate, proto->value_as_BooleanValueProto()->value())));
                    }
                    case ValueUnionProto::StringValueProto: {
                        const auto str_arg = proto->value_as_StringValueProto()->value();
                        auto str = v8::String::NewFromUtf8(isolate, str_arg->data(), v8::NewStringType::kNormal, str_arg->size()).ToLocalChecked();
                        return Result::Ok(handle_scope.Escape(str));
                    }
                    case ValueUnionProto::NumberValueProto: {
                        return Result::Ok(handle_scope.Escape(v8::Number::New(isolate, proto->value_as_NumberValueProto()->value())));
                    }
                    case ValueUnionProto::ObjectValueProto: {
                        const auto object_proto = proto->value_as_ObjectValueProto();
                        auto object = v8::ObjectTemplate::New(isolate)->NewInstance(context).ToLocalChecked();
                        if (object_proto->properties() && object_proto->properties()->size()) {
                            for (auto i = 0; i < object_proto->properties()->size(); ++i) {
                                const auto *prop_proto = object_proto->properties()->Get(i);
                                auto value_r = deserialize(log_context, txn, isolate, prop_proto->value());
                                if (!value_r)
                                    return Result::Error(value_r.get_error());
                                const auto n = prop_proto->name();
                                auto name = v8::String::NewFromUtf8(isolate, n->data(), v8::NewStringType::kNormal, n->size()).ToLocalChecked();
                                if (!object->Set(context, name, value_r.unwrap()).ToChecked()) {
                                    log_error(log_context, "Unable to set object value");
                                    return Result::Error(Code::ScriptEngine_UnableToSetObjectValue);
                                }
                            }
                        }
                        return Result::Ok(handle_scope.Escape(object));
                    }
                    case ValueUnionProto::ArrayValueProto: {
                        const auto array_proto = proto->value_as_ArrayValueProto();
                        if (array_proto->items() && array_proto->items()->size()) {
                            auto array = v8::Array::New(isolate, array_proto->items()->size());
                            for (auto i = 0; i < array_proto->items()->size(); ++i) {
                                const auto *item = array_proto->items()->Get(i);
                                auto value_r = deserialize(log_context, txn, isolate, item->value());
                                if (!value_r)
                                    return Result::Error(value_r.get_error());
                                auto index = item->index();
                                if (!array->Set(context, index, value_r.unwrap()).ToChecked()) {
                                    log_error(log_context, "Unable to set array value at index {}", index);
                                    return Result::Error(Code::ScriptEngine_UnableToSetArrayValue);
                                }
                            }
                            return Result::Ok(handle_scope.Escape(array));
                        } else {
                            return Result::Ok(handle_scope.Escape(v8::Array::New(isolate)));
                        }
                    }
                    case ValueUnionProto::DataReferenceValueProto: {
                        const auto arg = proto->value_as_DataReferenceValueProto();
                        auto ref = make_object_reference(data::ObjectType::WORKER_OBJECT, arg->class_id(),
                                                         PrimaryKey{arg->primary_key()->string_view()});
                        auto obj_r = js_load_object<data::ObjectType::WORKER_OBJECT>(isolate, ref, false);
                        if (!obj_r) {
                            auto error = obj_r.get_error();
                            if (error.is_code() && error.get_code() == Code::Datastore_ObjectDeleted) {
                                return Result::Ok(handle_scope.Escape(v8::Null(isolate)));
                            }
                            return Result::Error(std::move(error));
                        }
                        return Result::Ok(handle_scope.Escape(obj_r.unwrap()));
                    }
                    case ValueUnionProto::ServiceReferenceValueProto: {
                        const auto arg = proto->value_as_ServiceReferenceValueProto();
                        auto ref = make_object_reference(data::ObjectType::WORKER_SERVICE, arg->class_id(),
                                                         PrimaryKey{arg->primary_key()->string_view()});
                        auto obj_r = js_load_object<data::ObjectType::WORKER_SERVICE>(isolate, ref, false);
                        if (!obj_r) {
                            auto error = obj_r.get_error();
                            if (error.is_code() && error.get_code() == Code::Datastore_ObjectDeleted) {
                                return Result::Ok(handle_scope.Escape(v8::Null(isolate)));
                            }
                            return Result::Error(std::move(error));
                        }
                        return Result::Ok(handle_scope.Escape(obj_r.unwrap()));
                    }
                    case ValueUnionProto::MapValueProto: {
                        const auto map_proto = proto->value_as_MapValueProto();
                        if (map_proto->items() && map_proto->items()->size()) {
                            auto map = v8::Map::New(isolate);
                            for (auto i = 0; i < map_proto->items()->size(); ++i) {
                                const auto item = map_proto->items()->Get(i);
                                auto key_r = deserialize(log_context, txn, isolate, item->key());
                                if (!key_r)
                                    return Result::Error(key_r.get_error());
                                auto value_r = deserialize(log_context, txn, isolate, item->value());
                                if (!value_r)
                                    return Result::Error(value_r.get_error());
                                map->Set(context, key_r.unwrap(), value_r.unwrap()).ToLocalChecked();
                            }
                            return Result::Ok(handle_scope.Escape(map));
                        } else {
                            return Result::Ok(handle_scope.Escape(v8::Map::New(isolate)));
                        }
                    }
                    case ValueUnionProto::SetValueProto: {
                        const auto set_proto = proto->value_as_SetValueProto();
                        if (set_proto->items() && set_proto->items()->size()) {
                            auto set = v8::Set::New(isolate);
                            for (auto i = 0; i < set_proto->items()->size(); ++i) {
                                const auto item = set_proto->items()->Get(i);
                                auto value_r = deserialize(log_context, txn, isolate, item);
                                if (!value_r)
                                    return Result::Error(value_r.get_error());
                                set->Add(context, value_r.unwrap()).ToLocalChecked();
                            }
                            return Result::Ok(handle_scope.Escape(set));
                        } else {
                            return Result::Ok(handle_scope.Escape(v8::Set::New(isolate)));
                        }
                        break;
                    }
                    case ValueUnionProto::DateValueProto: {
                        const auto date_proto = proto->value_as_DateValueProto();
                        return Result::Ok(handle_scope.Escape(v8::Date::New(context, date_proto->value()).ToLocalChecked()));
                    }
                    default: {
                        log_critical(log_context, "Unknown value type when deserializing");
                        assert(false);
                        return Result::Error(Code::ScriptEngine_UnsupportedValueType);
                    }
                }
            }
            // - Must never have called tracker->reset_new()
            // Returns the number of deltas that were added to the call_context.
            EngineResultCode<std::size_t> save_objects(bool graph, CallContextS call_context, data::TrackerS tracker) {
                using Result = EngineResultCode<std::size_t>;
                assert(tracker->all_handle_count() == tracker->new_handle_count()); //must never have called reset_new()

                auto working_set = call_context->get_working_set();

                std::vector<data::ObjectS> changed_objects{};
                while (true) {
                    const auto new_handles = tracker->extract_new_handles();
                    if (new_handles.empty())
                        break;
                    for (const auto handle: new_handles) {
                        assert(handle->get_reference()->type == data::ObjectType::WORKER_OBJECT);
                        UNWRAP_OR_RETURN(object, working_set->resolve(handle));
                        UNWRAP_OR_RETURN(changed, object->flush_and_save(graph ? std::make_optional(tracker) : std::nullopt));
                        if (changed) {
                            changed_objects.push_back(std::move(object));
                        }
                    }
                }

                auto delta_count{0};
                if (!changed_objects.empty()) {
                    flatbuffers::FlatBufferBuilder delta_builder{};
                    const auto log_context = call_context->get_log_context();
                    auto buffer_pool = call_context->get_buffer_pool();
                    auto builder = call_context->get_reusable_builder(true);
                    auto deltas = data::create_deltas(log_context, buffer_pool, builder, changed_objects);
                    delta_count = deltas.size();
                    for (auto &[handle, delta]: deltas) {
                        call_context->add_delta(std::move(delta));
                    }
                }
                return Result::Ok(delta_count);
            }
            namespace native {
                void noop(const v8::FunctionCallbackInfo<v8::Value> &args) {}
                namespace runtime {
                    namespace detail {
                        template<bool NEW, data::ObjectType OT>
                        void constructor(const char *from, const v8::FunctionCallbackInfo<v8::Value> &args) {
                            static const std::string TYPE_NAME = OT == data::ObjectType::WORKER_OBJECT ? "Data" : "Service";
                            V8_SCOPE(args.GetIsolate());

                            auto engine = get_engine(isolate);
                            auto call_context = engine->get_call_context();
                            const auto &log_context = call_context->get_log_context();

                            if (args.Length() != 1) {
                                V8_THROW(from, "Exactly one argument is required: primaryKey");
                                log_error(log_context, "Failure in {}: {}: {}", __PRETTY_FUNCTION__, error_message, args.Length());
                                return;
                            }

                            if (!args.IsConstructCall()) {
                                V8_THROW_FMT(from, "Unable to call {} constructor as a function", TYPE_NAME);
                                log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                return;
                            }

                            //Single argument: primaryKey
                            v8::Local<v8::String> primary_key_string{};
                            if (!args[0]->ToString(context).ToLocal(&primary_key_string)) {
                                V8_THROW(from, "Invalid primaryKey");
                                log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                return;
                            }
                            PrimaryKey primary_key{value_to_string(isolate, primary_key_string)};

                            //doing this as late as possible as it's probably heavier than the other preconditions.
                            const auto maybe_class_id = engine->maybe_get_derived_class_id<OT>(isolate, args.This());
                            if (!maybe_class_id) {
                                V8_THROW_FMT(from, "Unable to find the correct class id to {} {}.",
                                             NEW ? "create a new" : "load an existing",
                                             TYPE_NAME);
                                log_error(log_context, "Failure in {}: {}: {}", __PRETTY_FUNCTION__, error_message, args.Length());
                                return;
                            }
                            const auto class_id = maybe_class_id.value();

                            auto working_set = call_context->get_working_set();
                            auto ref = make_object_reference(OT, class_id, std::move(primary_key));

                            data::ObjectS object;
                            if (NEW) {
                                auto object_r = working_set->create(ref);
                                if (!object_r) {
                                    engine->set_internal_error(object_r.get_error());
                                    V8_THROW_FMT(from, "Unable to create new {} due to error {}", TYPE_NAME, get_code_name(object_r.get_error()));
                                    log_warn(log_context, "Failure in {}. Message: {}", __PRETTY_FUNCTION__, error_message);
                                    return;
                                }
                                object = object_r.unwrap();
                            } else {
                                auto object_r = working_set->resolve(ref, OT == data::ObjectType::WORKER_SERVICE);
                                if (!object_r) {
                                    engine->set_internal_error(object_r.get_error());
                                    V8_THROW_FMT(from, "Unable to apply {} due to error {}", TYPE_NAME, get_code_name(object_r.get_error()));
                                    log_warn(log_context, "Failure in {}. Message: {}", __PRETTY_FUNCTION__, error_message);
                                    return;
                                }
                                object = object_r.unwrap();
                            }

                            auto target = args.This();
                            assert(target->InternalFieldCount() == 1);
                            target->SetInternalField(0, v8::External::New(isolate, object.get()));
                            args.GetReturnValue().Set(target);
                        }
                    }
                    void ESTATE_NEW_MESSAGE_CTOR(const v8::FunctionCallbackInfo<v8::Value> &args) {
                        //sj note: Noop. not using native::noop because I like the consistency of having all the class ctors
                        // defined and if they're going to be defined they should be used (even if it's noop).
                    }
                    void ESTATE_NEW_DATA_CTOR(const v8::FunctionCallbackInfo<v8::Value> &args) {
                        detail::constructor<true, data::ObjectType::WORKER_OBJECT>(ESTATE_NEW_DATA_CTOR_STR, args);
                    }
                    void ESTATE_NEW_SERVICE_CTOR(const v8::FunctionCallbackInfo<v8::Value> &args) {
                        detail::constructor<true, data::ObjectType::WORKER_SERVICE>(ESTATE_NEW_SERVICE_CTOR_STR, args);
                    }
                    void ESTATE_CREATE_UUID_FUNCTION(const v8::FunctionCallbackInfo<v8::Value> &args) {
                        V8_SCOPE(args.GetIsolate());
                        static const char *FROM = ESTATE_CREATE_UUID_FUNCTION_STR;

                        auto engine = get_engine(isolate);
                        auto call_context = engine->get_call_context();
                        const auto &log_context = call_context->get_log_context();

                        bool dashes;
                        if (args.Length() == 1) {
                            dashes = args[0]->ToBoolean(isolate)->Value();
                        } else if (args.Length() == 0) {
                            dashes = false;
                        } else {
                            V8_THROW(FROM, "Incorrect number of arguments");
                            log_error(log_context, "Failure in {}: {}: {}", __PRETTY_FUNCTION__, error_message, args.Length());
                            return;
                        }

                        using namespace boost::uuids;
                        static random_generator random_generator{};
                        uuid u = random_generator();

                        std::string result;
                        result.reserve(36);

                        std::size_t i = 0;
                        for (uuid::const_iterator it_data = u.begin(); it_data != u.end(); ++it_data, ++i) {
                            const size_t hi = ((*it_data) >> 4) & 0x0F;
                            result += boost::uuids::detail::to_char(hi);

                            const size_t lo = (*it_data) & 0x0F;
                            result += boost::uuids::detail::to_char(lo);

                            if (dashes && (i == 3 || i == 5 || i == 7 || i == 9)) {
                                result += '-';
                            }
                        }

                        auto result_value = V8_STRING(result.data(), result.size());
                        args.GetReturnValue().Set(result_value);
                    }
                    namespace system {
                        namespace detail {
                            template<data::ObjectType OT>
                            void get_object(const char *from, const v8::FunctionCallbackInfo<v8::Value> &args) {
                                static const std::string TYPE_NAME = OT == data::ObjectType::WORKER_OBJECT ? "Data" : "Service";
                                static const std::string SHORT_TYPE_NAME = OT == data::ObjectType::WORKER_OBJECT ? "object" : "service";

                                V8_SCOPE(args.GetIsolate());

                                auto engine = get_engine(isolate);
                                auto call_context = engine->get_call_context();
                                const auto &log_context = call_context->get_log_context();

                                if (args.Length() != 2) {
                                    V8_THROW(from, "Incorrect number of arguments");
                                    log_error(log_context, "Failure in {}: {}: {}", __PRETTY_FUNCTION__, error_message, args.Length());
                                    return;
                                }

                                //First argument: classType
                                if (!args[0]->IsFunction()) {
                                    V8_THROW(from, "Invalid value for class type");
                                    log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                    return;
                                }
                                const auto class_name = value_to_string(isolate, v8::Local<v8::Function>::Cast(args[0])->GetName());
                                auto class_id_v = engine->get_class_id<data::get_class_type<OT>()>(class_name);
                                if (std::holds_alternative<ClassLookupCode>(class_id_v)) {
                                    switch (std::get<ClassLookupCode>(class_id_v)) {
                                        case ClassLookupCode::WRONG_CLASS_TYPE: {
                                            V8_THROW_FMT(from, "The class {} does not extend {}", class_name, TYPE_NAME);
                                            log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                            return;
                                        }
                                        case ClassLookupCode::NOT_FOUND: {
                                            V8_THROW_FMT(from, "The class {} does not exist", class_name, TYPE_NAME);
                                            log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                            return;
                                        }
                                        default:
                                            assert(false);
                                            return;
                                    }
                                }

                                const auto class_id = std::get<ClassId>(class_id_v);

                                //Second argument: primaryKey
                                v8::Local<v8::String> primary_key_string{};
                                if (!args[1]->ToString(context).ToLocal(&primary_key_string)) {
                                    V8_THROW(from, "invalid primaryKey");
                                    log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                    return;
                                }

                                const auto ref = make_object_reference(OT, class_id, PrimaryKey{value_to_string(isolate, primary_key_string)});

                                std::optional<EngineError> maybe_error{};
                                v8::Local<v8::Object> object;
                                {
                                    v8::TryCatch try_catch{isolate};
                                    auto object_r = js_load_object<OT>(isolate, ref, true);
                                    if (!object_r) {
                                        if (try_catch.HasCaught()) {
                                            assert(object_r.get_error().is_exception());
                                            try_catch.ReThrow();
                                            return;
                                        } else {
                                            maybe_error.emplace(std::move(object_r.get_error()));
                                        }
                                    } else {
                                        assert(!try_catch.HasCaught());
                                        object = object_r.unwrap();
                                    }
                                }
                                if (maybe_error.has_value()) {
                                    const auto error = std::move(maybe_error.value());
                                    assert(error.is_code());
                                    V8_THROW_FMT(from, "Unable to load {} due to error {}", SHORT_TYPE_NAME, get_code_name(error.get_code()));
                                    log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                    return;
                                }

                                args.GetReturnValue().Set(object);
                            }
                            void save(const char *from, bool graph, const v8::FunctionCallbackInfo<v8::Value> &args) {
                                V8_SCOPE(args.GetIsolate());

                                auto engine = get_engine(isolate);
                                auto call_context = engine->get_call_context();
                                const auto &log_context = call_context->get_log_context();

                                if (args.Length() == 0) {
                                    V8_THROW(from, "One or more objects required");
                                    log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                    return;
                                }

                                auto tracker = data::Tracker::Create();

                                for (auto i = 0; i < args.Length(); ++i) {
                                    if (args[i]->IsArray()) {
                                        const auto arr = v8::Local<v8::Array>::Cast(args[i]);
                                        for (auto j = 0; j < arr->Length(); ++j) {
                                            const auto arr_value = arr->Get(context, j).ToLocalChecked();
                                            if (!arr_value->IsObject()) {
                                                V8_THROW_FMT(from, "Invalid array item {} in argument at position {}", j + 1, i + 1);
                                                log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                                return;
                                            }
                                            auto object_value = v8::Local<v8::Object>::Cast(arr_value);
                                            if (object_value->InternalFieldCount() == 0) {
                                                V8_THROW_FMT(from,
                                                             "Invalid array item {} in argument at position {}: not an instance of a managed worker type (specifically, Data)",
                                                             j + 1, i + 1);
                                                log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                                return;
                                            }
                                            auto object = V8_UNWRAP_OBJECT(object_value);
                                            if (object->get_reference()->type != data::ObjectType::WORKER_OBJECT) {
                                                V8_THROW_FMT(from, "Invalid array item {} in argument at position {}: not a Data instance",
                                                             j + 1, i + 1);
                                                log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                                return;
                                            }

                                            tracker->track(object->get_handle());
                                        }
                                    } else {
                                        if (!args[i]->IsObject()) {
                                            V8_THROW_FMT(from, "Invalid argument at position {}", i + 1);
                                            log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                            return;
                                        }
                                        auto object_value = v8::Local<v8::Object>::Cast(args[i]);
                                        if (object_value->InternalFieldCount() == 0) {
                                            V8_THROW_FMT(from,
                                                         "Invalid argument at position {}: not an instance of a managed worker type (specifically, Data)",
                                                         i + 1);
                                            log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                            return;
                                        }
                                        auto object = V8_UNWRAP_OBJECT(object_value);
                                        if (object->get_reference()->type != data::ObjectType::WORKER_OBJECT) {
                                            V8_THROW_FMT(from, "Invalid argument at position {}: not a Data instance", i + 1);
                                            log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                            return;
                                        }

                                        tracker->track(object->get_handle());
                                    }
                                }

                                auto delta_count{0};
                                {
                                    std::optional<Code> error_code{};
                                    {
                                        v8::TryCatch try_catch{isolate};
                                        auto delta_count_r = save_objects(graph, call_context, tracker);
                                        if (!delta_count_r) {
                                            if (try_catch.HasCaught()) {
                                                assert(delta_count_r.get_error().is_exception());
                                                try_catch.ReThrow();
                                                return;
                                            } else {
                                                assert(delta_count_r.get_error().is_code());
                                                error_code = delta_count_r.get_error().get_code();
                                            }
                                        }
                                        assert(!try_catch.HasCaught());
                                        delta_count = delta_count_r.unwrap();
                                    }
                                    if (error_code.has_value()) {
                                        V8_THROW(from, "Internal error while saving objects");
                                        log_error(log_context, "Failure in {} '{}' code: {}", __PRETTY_FUNCTION__, error_message,
                                                  get_code_name(error_code.value()));
                                        return;
                                    }
                                }

                                if (delta_count > 0)
                                    log_trace(log_context, "{} saved {} objects and generated {} deltas", __PRETTY_FUNCTION__,
                                              tracker->all_handle_count(), delta_count);
                                else
                                    log_trace(log_context, "{} didn't save {} objects because there were no changes", __PRETTY_FUNCTION__,
                                              tracker->all_handle_count());

                                args.GetReturnValue().Set(v8::Boolean::New(isolate, delta_count > 0));
                            }
                        }
                        // Creates a UUID with or without dashes
                        void ESTATE_REVERT_FUNCTION(const v8::FunctionCallbackInfo<v8::Value> &args) {
                            V8_SCOPE(args.GetIsolate());
                            static const char *FROM = ESTATE_REVERT_FUNCTION_STR;

                            auto engine = get_engine(isolate);
                            auto call_context = engine->get_call_context();
                            const auto &log_context = call_context->get_log_context();

                            if (args.Length() != 1) {
                                V8_THROW(FROM, "Incorrect number of arguments");
                                log_error(log_context, "Failure in {}: {}: {}", __PRETTY_FUNCTION__, error_message, args.Length());
                                return;
                            }

                            if (!args[0]->IsObject()) {
                                V8_THROW(FROM, "Invalid argument");
                                log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                return;
                            }

                            auto object_value = v8::Local<v8::Object>::Cast(args[0]);
                            if (object_value->InternalFieldCount() == 0) {
                                V8_THROW(FROM, "Invalid argument: not an instance of a managed worker type (specifically, Data or Service)");
                                log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                return;
                            }

                            auto object = V8_UNWRAP_OBJECT(object_value);
                            object->revert();
                        }
                        // Gets an existing Service given its type and primaryKey.
                        void ESTATE_GET_SERVICE_FUNCTION(const v8::FunctionCallbackInfo<v8::Value> &args) {
                            detail::get_object<data::ObjectType::WORKER_SERVICE>(ESTATE_GET_SERVICE_FUNCTION_STR, args);
                        }
                        // Gets an existing Data instance given its type and primaryKey
                        void ESTATE_GET_DATA_FUNCTION(const v8::FunctionCallbackInfo<v8::Value> &args) {
                            detail::get_object<data::ObjectType::WORKER_OBJECT>(ESTATE_GET_DATA_FUNCTION_STR, args);
                        }
                        // Deletes an object or service.
                        void ESTATE_DELETE_FUNCTION(const v8::FunctionCallbackInfo<v8::Value> &args) {
                            static const char *FROM = ESTATE_DELETE_FUNCTION_STR;

                            V8_SCOPE(args.GetIsolate());

                            auto engine = get_engine(isolate);
                            auto call_context = engine->get_call_context();
                            const auto &log_context = call_context->get_log_context();

                            const auto args_length = args.Length();
                            if (args_length > 2 || args_length < 1) {
                                V8_THROW(FROM, "Incorrect number of arguments.");
                                log_error(log_context, "Failure in {}: {}: {}", __PRETTY_FUNCTION__, error_message, args.Length());
                                return;
                            }

                            const auto bad_obj_msg = "Invalid argument 1. Must be an object instance of a class derived from either Service or Data.";

                            if (!args[0]->IsObject()) {
                                V8_THROW(FROM, bad_obj_msg);
                                log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                return;
                            }

                            auto object_value = v8::Local<v8::Object>::Cast(args[0]);
                            if (object_value->InternalFieldCount() == 0) {
                                V8_THROW(FROM, bad_obj_msg);
                                log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                return;
                            }
                            auto object = V8_UNWRAP_OBJECT(object_value);

                            bool purge{false};
                            if (args_length == 2) {
                                if (!args[1]->IsBoolean() && !args[1]->IsBooleanObject()) {
                                    V8_THROW(FROM, "Invalid argument 2: not a boolean value.");
                                    log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                    return;
                                }
                                auto purge_value = v8::Local<v8::Boolean>::Cast(args[1]);
                                purge = purge_value->BooleanValue(isolate);
                            }

                            if (object->set_is_deleted(purge) && object->is_data()) {
                                auto tracker = data::Tracker::Create();
                                tracker->track(object->get_handle());
                                std::optional<Code> error_code{};
                                {
                                    v8::TryCatch try_catch{isolate};
                                    auto delta_count_r = save_objects(false, call_context, tracker);
                                    if (!delta_count_r) {
                                        if (try_catch.HasCaught()) {
                                            assert(delta_count_r.get_error().is_exception());
                                            try_catch.ReThrow();
                                            return;
                                        } else {
                                            assert(delta_count_r.get_error().is_code());
                                            error_code = delta_count_r.get_error().get_code();
                                        }
                                    }
                                    assert(!try_catch.HasCaught());
                                }
                                if (error_code.has_value()) {
                                    V8_THROW_FMT(FROM, "Unable to save Data changes (after deletion) due to the error {}",
                                                 get_code_name(error_code.value()));
                                    log_error(log_context, "Failure in {} '{}' code: {}", __PRETTY_FUNCTION__, error_message,
                                              get_code_name(error_code.value()));
                                    return;
                                }
                            }
                        }
                        // Saves all the objects passed in and all the objects they reference.
                        // Returns true if any of the objects had changes to save
                        void ESTATE_SAVE_DATA_GRAPHS_FUNCTION(const v8::FunctionCallbackInfo<v8::Value> &args) {
                            detail::save(ESTATE_SAVE_DATA_GRAPHS_FUNCTION_STR, true, args);
                        }
                        // Saves all the objects passed in.
                        // Returns true if any of the objects had changes to save
                        void ESTATE_SAVE_DATA_FUNCTION(const v8::FunctionCallbackInfo<v8::Value> &args) {
                            detail::save(ESTATE_SAVE_DATA_FUNCTION_STR, false, args);
                        }
                        // Fires a Message instance to all clients that are listening.
                        void ESTATE_SEND_MESSAGE_FUNCTION(const v8::FunctionCallbackInfo<v8::Value> &args) {
                            V8_SCOPE(args.GetIsolate());
                            static const char *FROM = ESTATE_SEND_MESSAGE_FUNCTION_STR;

                            auto *engine = get_engine(isolate);
                            auto call_context = engine->get_call_context();
                            const auto &log_context = call_context->get_log_context();

                            if (args.Length() != 2) {
                                V8_THROW(FROM, "Invalid arguments. Two arguments are required: source, and message.");
                                log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                return;
                            }

                            // process source argument
                            if (!args[0]->IsObject()) {
                                V8_THROW(FROM,
                                         "Invalid source argument: must be an instance of a class that extends either Service or Data.");
                                log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                return;
                            }

                            auto source_object_value = v8::Local<v8::Object>::Cast(args[0]);
                            if (source_object_value->InternalFieldCount() == 0) {
                                V8_THROW(FROM,
                                         "Invalid source argument: must be an instance of a class that extends either Service or Data.");
                                log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                return;
                            }
                            auto source_object = V8_UNWRAP_OBJECT(source_object_value);
                            if (source_object->is_deleted()) {
                                V8_THROW(FROM, "Invalid source argument: deleted");
                                log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                return;
                            }

                            // process message argument
                            if (!args[1]->IsObject()) {
                                V8_THROW(FROM, "Invalid message argument: must be an instance of a class that extends Message.");
                                log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                return;
                            }

                            auto event_object = v8::Local<v8::Object>::Cast(args[1]);
                            auto class_name = value_to_string(isolate, event_object->GetConstructorName());
                            auto class_id_v = engine->get_class_id<data::ClassType::WORKER_EVENT>(class_name);
                            if (std::holds_alternative<ClassLookupCode>(class_id_v)) {
                                switch (std::get<ClassLookupCode>(class_id_v)) {
                                    case ClassLookupCode::WRONG_CLASS_TYPE: {
                                        V8_THROW(FROM, "Invalid argument: the object extends the wrong type. It must extend Message.");
                                        log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                        return;
                                    }
                                    case ClassLookupCode::NOT_FOUND: {
                                        V8_THROW(FROM,
                                                 "Invalid argument: the object is not an instance of a managed worker type. It must extend Message.");
                                        log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                        return;
                                    }
                                    default:
                                        assert(false);
                                        return;
                                }
                            }
                            auto class_id = std::get<ClassId>(class_id_v);

                            fbs::Builder builder{};

                            auto names = event_object->GetOwnPropertyNames(context).ToLocalChecked();
                            const auto names_len = names->Length();
                            if (names_len == 0) {
                                call_context->add_fired_event(finish_and_copy_to_buffer(builder, call_context->get_buffer_pool(),
                                                                                        CreateMessageProtoDirect(builder, class_id)));
                                log_trace(log_context, "Added event {} with no properties, no referenced worker objects, and no deltas.", class_name);
                                return;
                            }

                            builder.Reset();

                            auto tracker = data::Tracker::Create();

                            // Serialize all the event's properties
                            std::vector<fbs::Offset<PropertyProto>> event_properties{};
                            for (auto i = 0; i < names_len; ++i) {
                                auto n = names->Get(context, i).ToLocalChecked();
                                auto v = event_object->Get(context, n).ToLocalChecked();
                                ValueUnionProto unused;
                                auto value_off_r = serialize(log_context, isolate, builder, v, unused, tracker);
                                if (!value_off_r) {
                                    if (value_off_r.get_error() == Code::ScriptEngine_UnsupportedValueType) {
                                        V8_THROW_FMT(FROM,
                                                     "Unable to serialize the event object because its property {} either was or contained a property of an unsupported value type.",
                                                     value_to_string(isolate, n));
                                        log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                        return;
                                    } else {
                                        V8_THROW_FMT(FROM, "Unable to serialize event object due to the internal error {}",
                                                     get_code_name(value_off_r.get_error()));
                                        log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                        return;
                                    }
                                }
                                auto value_off = value_off_r.unwrap();
                                auto name_str = value_to_string(isolate, n);
                                auto name = builder.CreateString(name_str.data(), name_str.size());
                                event_properties.push_back(CreatePropertyProto(builder, name, value_off));
                            }

                            auto delta_count{0};
                            std::vector<fbs::Offset<DataHandleProto>> referenced_data_handles{};
                            if (tracker->all_handle_count() > 0) {
                                auto working_set = call_context->get_working_set();
                                for (const auto &handle_wrapper: tracker->all_handles()) {
                                    const auto handle = handle_wrapper.handle;
                                    auto object_r = working_set->resolve(handle);
                                    if (!object_r) {
                                        V8_THROW(FROM,
                                                 "An internal error occurred while firing an event. The handle couldn't be found in the working set.");
                                        log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                        return;
                                    }
                                    auto object = object_r.unwrap();

                                    auto maybe_class_name_type = engine->maybe_get_class_name_type(handle->get_reference()->class_id);
                                    if (!maybe_class_name_type) {
                                        V8_THROW(FROM,
                                                 "An internal error occurred while firing an event. The handle's class ID couldn't be resolved to a class name.");
                                        log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                        return;
                                    }
                                    const auto class_name_type = std::move(maybe_class_name_type.value());

                                    auto was_already_saved_r = object->was_already_saved();
                                    if (!was_already_saved_r) {
                                        V8_THROW_FMT(FROM,
                                                     "An internal error occurred while firing an event. Could not verify the object of type {} had been saved already.",
                                                     class_name_type.first);
                                        log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                        return;
                                    }
                                    const auto was_already_saved = was_already_saved_r.unwrap();
                                    if (!was_already_saved) {
                                        assert(class_name_type.second != data::ClassType::WORKER_EVENT);
                                        const bool is_object = class_name_type.second == data::ClassType::WORKER_OBJECT;
                                        V8_THROW_FMT(FROM,
                                                     "Unable to fire event because the referenced {} (class: {}, primaryKey: {}) had unsaved changes. All Service and Data instances an event references that have been changed as part of the service method call must be manually saved before firing.",
                                                     is_object ? "Data" : "Service",
                                                     class_name_type.first,
                                                     handle->get_reference()->get_primary_key().view());
                                        log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                        return;
                                    }
                                }

                                for (const auto &handle_wrapper: tracker->all_handles()) {
                                    const auto handle = handle_wrapper.handle;
                                    referenced_data_handles.push_back(
                                            CreateDataHandleProto(builder, handle->get_reference()->class_id, handle->get_version(),
                                                                        builder.CreateString(handle->get_reference()->get_primary_key().view())));
                                }
                            }

                            WorkerReferenceUnionProto source_type{};
                            fbs::Offset<void> source{};
                            const auto source_reference = source_object->get_reference();
                            if (source_reference->type == data::ObjectType::WORKER_OBJECT) {
                                source_type = WorkerReferenceUnionProto::DataReferenceValueProto;
                                source = CreateDataReferenceValueProto(builder, source_reference->class_id, builder.CreateString(
                                        source_reference->get_primary_key().view())).Union();
                            } else {
                                source_type = WorkerReferenceUnionProto::ServiceReferenceValueProto;
                                source = CreateServiceReferenceValueProto(builder, source_reference->class_id, builder.CreateString(
                                        source_reference->get_primary_key().view())).Union();
                            }

                            call_context->add_fired_event(
                                    finish_and_copy_to_buffer(builder, call_context->get_buffer_pool(),
                                                              CreateMessageProtoDirect(
                                                                      builder,
                                                                      class_id,
                                                                      source_type,
                                                                      source,
                                                                      event_properties.empty() ? nullptr : &event_properties,
                                                                      referenced_data_handles.empty() ? nullptr
                                                                                                             : &referenced_data_handles
                                                              )));

                            log_trace(log_context, "Added event {} with {} properties, {} referenced worker objects, and {} object deltas",
                                      class_name, event_properties.size(), referenced_data_handles.size(), delta_count);
                        }
                    }
                }
                namespace runtime_internal {
                    void ESTATE_EXISTING_DATA_CTOR(const v8::FunctionCallbackInfo<v8::Value> &args) {
                        native::runtime::detail::constructor<false, data::ObjectType::WORKER_OBJECT>(ESTATE_EXISTING_DATA_CTOR_STR, args);
                    }
                    void ESTATE_EXISTING_SERVICE_CTOR(const v8::FunctionCallbackInfo<v8::Value> &args) {
                        native::runtime::detail::constructor<false, data::ObjectType::WORKER_SERVICE>(ESTATE_EXISTING_SERVICE_CTOR_STR, args);
                    }
                }
                namespace console {
                    namespace detail {
                        void write(const char *from, bool error, const v8::FunctionCallbackInfo<v8::Value> &args) {
                            V8_SCOPE(args.GetIsolate());

                            auto engine = get_engine(isolate);
                            auto call_context = engine->get_call_context();
                            const auto &log_context = call_context->get_log_context();

                            std::string message{};
                            for (int i = 0; i < args.Length(); ++i) {
                                v8::Local<v8::String> message_value{};
                                if (!args[i]->ToString(context).ToLocal(&message_value)) {
                                    assert(false); //should never happen
                                    V8_THROW_FMT(from, "Invalid item passed to {} call at index {}", error ? "error" : "log", i);
                                    log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                    return;
                                }
                                const auto msg = value_to_string(isolate, message_value);
                                if (message.empty()) {
                                    message = msg;
                                } else {
                                    message.append(" ");
                                    message.append(msg);
                                }
                            }

                            if (message.empty())
                                return;

                            if (error)
                                call_context->get_console_log()->append_error(std::move(message));
                            else
                                call_context->get_console_log()->append_log(std::move(message));
                        }
                    }
                    void ESTATE_CONSOLE_LOG_FUNCTION(const v8::FunctionCallbackInfo<v8::Value> &args) {
                        detail::write(ESTATE_CONSOLE_LOG_FUNCTION_STR, false, args);
                    }
                    void ESTATE_CONSOLE_ERROR_FUNCTION(const v8::FunctionCallbackInfo<v8::Value> &args) {
                        detail::write(ESTATE_CONSOLE_ERROR_FUNCTION_STR, true, args);
                    }
                }
                namespace object {
                    namespace detail {
                        static const std::string PRIMARY_KEY_PROPERTY_NAME{"primaryKey"};

                        template<data::ObjectType>
                        void on_property_get(const char *from, v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value> &info) {
                            V8_ESCAPABLE_SCOPE(info.GetIsolate());

                            //unwrap the object
                            data::Object *object = V8_UNWRAP_OBJECT(info.This());
                            auto *engine = get_engine(isolate);
                            auto call_context = engine->get_call_context();
                            const auto &log_context = call_context->get_log_context();

                            //convert the name to a string I can use
                            v8::String::Utf8Value name_val_str(isolate, name);
                            const std::string name_str{*name_val_str, static_cast<size_t>(name_val_str.length())};

                            if (name->IsSymbol()) {
                                log_warn(log_context, "Name {0} is a Symbol, skipping read operation", name_str);
                                return;
                            }

                            const auto ref = object->get_reference();

                            //if they're asking for the primary key, return that.
                            if (name_str == PRIMARY_KEY_PROPERTY_NAME) {
                                const auto pk = ref->get_primary_key().view();
                                info.GetReturnValue().Set(handle_scope.Escape(V8_STRING(pk.data(), pk.size())));
                                return;
                            }

                            std::optional<MethodKindProto> method_kind;
                            if (ref->is_data()) {
                                method_kind = engine->object_has_getter_or_normal_method(ref->class_id, name_str);
                            } else {
                                if (engine->service_has_method(ref->class_id, name_str))
                                    method_kind = MethodKindProto::Normal;
                            }

                            if (method_kind) {
                                auto object_proto = info.This()->GetPrototype()->ToObject(context).ToLocalChecked();
                                if (method_kind.value() == MethodKindProto::Getter) {
                                    //Get the getter function and call it
                                    v8::Local<v8::Object> method_descriptor = v8::Local<v8::Object>::Cast(
                                            object_proto->GetOwnPropertyDescriptor(context, name).ToLocalChecked());
                                    static const std::string GET{"get"};
                                    v8::Local<v8::Value> get_val = v8::String::NewFromUtf8(isolate, GET.data(), v8::NewStringType::kNormal,
                                                                                           GET.size()).ToLocalChecked();
                                    auto get_method = v8::Local<v8::Function>::Cast(method_descriptor->Get(context, get_val).ToLocalChecked());
                                    auto value = get_method->CallAsFunction(context, info.This(), 0, nullptr).ToLocalChecked();
                                    info.GetReturnValue().Set(value);
                                    return;
                                } else if (method_kind.value() == MethodKindProto::Normal) {
                                    auto method = v8::Local<v8::Function>::Cast(object_proto->Get(context, name).ToLocalChecked());
                                    info.GetReturnValue().Set(method);
                                    return;
                                }
                                assert(false);
                            }

                            auto prop_r = object->get_property(name_str);
                            if (!prop_r) {
                                V8_THROW(from, "Unable to get property when trying to read it");
                                log_error(log_context, "Failure in {} '{}' code: {}", __PRETTY_FUNCTION__, error_message,
                                          get_code_name(prop_r.get_error()));
                                return;
                            }
                            auto prop = prop_r.unwrap();

                            v8::Local<v8::Value> value{};
                            std::optional<Code> error_code{};
                            {
                                v8::TryCatch try_catch{isolate};
                                auto value_r = prop->get_script_value();
                                if (!value_r) {
                                    if (try_catch.HasCaught()) {
                                        assert(value_r.get_error().is_exception());
                                        try_catch.ReThrow();
                                        return;
                                    } else {
                                        assert(value_r.get_error().is_code());
                                        error_code = value_r.get_error().get_code();
                                    }
                                } else {
                                    value = value_r.unwrap();
                                }
                                assert(!try_catch.HasCaught());
                            }
                            if (error_code.has_value()) {
                                V8_THROW(from, "Unable to get property value");
                                log_error(log_context, "Failure in {} '{}' code: {}", __PRETTY_FUNCTION__, error_message,
                                          get_code_name(error_code.value()));
                                return;
                            }

                            info.GetReturnValue().Set(handle_scope.Escape(value));
                        }
                        template<data::ObjectType>
                        void on_property_set(const char *from, v8::Local<v8::Name> name, v8::Local<v8::Value> value,
                                             const v8::PropertyCallbackInfo<v8::Value> &info) {
                            V8_SCOPE(info.GetIsolate());

                            data::Object *object = V8_UNWRAP_OBJECT(info.Holder());

                            auto *engine = get_engine(isolate);
                            auto call_context = engine->get_call_context();
                            const auto &log_context = call_context->get_log_context();

                            v8::String::Utf8Value name_val_str(isolate, name);
                            std::string name_str{*name_val_str, static_cast<size_t>(name_val_str.length())};

                            if (name->IsSymbol()) {
                                log_error(log_context, "Name {0} is a Symbol, skipping read operation", name_str);
                                return;
                            }

                            if (name_str == PRIMARY_KEY_PROPERTY_NAME) {
                                V8_THROW(from, "Cannot change an object's primary key.");
                                log_warn(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                return;
                            }

                            const auto ref = object->get_reference();

                            if (ref->is_data()) {
                                if (engine->object_has_setter_method(ref->class_id, name_str)) {
                                    //Get the setter function and call it
                                    auto object_proto = info.This()->GetPrototype()->ToObject(context).ToLocalChecked();
                                    v8::Local<v8::Object> method_descriptor = v8::Local<v8::Object>::Cast(
                                            object_proto->GetOwnPropertyDescriptor(context, name).ToLocalChecked());
                                    static const std::string SET{"set"};
                                    v8::Local<v8::Value> set_str = v8::String::NewFromUtf8(isolate, SET.data(), v8::NewStringType::kNormal,
                                                                                           SET.size()).ToLocalChecked();
                                    auto set_method = v8::Local<v8::Function>::Cast(method_descriptor->Get(context, set_str).ToLocalChecked());
                                    set_method->Call(context, info.This(), 1, &value).ToLocalChecked();
                                    info.GetReturnValue().Set(false);
                                    return;
                                }
                            } else {
                                assert(ref->is_service());
                                if (engine->service_has_method(ref->class_id, name_str)) {
                                    V8_THROW(from, "Cannot set worker class function values at runtime");
                                    log_warn(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                    return;
                                }
                            }

                            auto prop_r = object->get_property(name_str);
                            if (!prop_r) {
                                V8_THROW(from, "Unable to get property when trying to set it");
                                log_error(log_context, "Failure in {} '{}' code: {}", __PRETTY_FUNCTION__, error_message,
                                          get_code_name(prop_r.get_error()));
                                return;
                            }
                            auto prop = prop_r.unwrap();
                            prop->set_script_value(value);
                        }
                        template<data::ObjectType>
                        void on_property_delete(const char *from, v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Boolean> &info) {
                            V8_SCOPE(info.GetIsolate());

                            data::Object *object = V8_UNWRAP_OBJECT(info.Holder());
                            auto *engine = get_engine(isolate);
                            auto call_context = engine->get_call_context();
                            const auto &log_context = call_context->get_log_context();

                            if (name->IsSymbol()) {
                                V8_THROW(from, "Unable to delete symbol");
                                log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                return;
                            }

                            v8::String::Utf8Value name_value_str(isolate, name);
                            std::string name_str{*name_value_str, static_cast<size_t>(name_value_str.length())};

                            if (name_str == PRIMARY_KEY_PROPERTY_NAME) {
                                V8_THROW(from, "Cannot delete an object's primary key.");
                                log_warn(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                return;
                            }

                            const auto &ref = object->get_reference();

                            bool has_method = false;
                            if (ref->is_data()) {
                                has_method = engine->object_has_method(ref->class_id, name_str);
                            } else {
                                assert(ref->is_service());
                                has_method = engine->service_has_method(ref->class_id, name_str);
                            }

                            if (has_method) {
                                V8_THROW(from, "Cannot delete worker class methods at runtime");
                                log_error(log_context, "Failure in {}: {}", __PRETTY_FUNCTION__, error_message);
                                return;
                            }

                            auto prop_r = object->get_property(name_str);
                            if (!prop_r) {
                                V8_THROW(from, "Unable to get property while trying to delete it");
                                log_error(log_context, "Failure in {} '{}' code: {}", __PRETTY_FUNCTION__, error_message,
                                          get_code_name(prop_r.get_error()));
                                return;
                            }

                            auto prop = prop_r.unwrap();

                            object->erase_property(prop, true);

                            info.GetReturnValue().Set(true);
                        }
                    }
                    void on_service_property_get(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value> &info) {
                        detail::on_property_get<data::ObjectType::WORKER_SERVICE>("on_service_property_get", std::move(name), info);
                    }
                    void
                    on_service_property_set(v8::Local<v8::Name> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value> &info) {
                        detail::on_property_set<data::ObjectType::WORKER_SERVICE>("on_service_property_set", std::move(name), std::move(value), info);
                    }
                    void on_service_property_delete(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Boolean> &info) {
                        detail::on_property_delete<data::ObjectType::WORKER_SERVICE>("on_service_property_delete", std::move(name), info);
                    }
                    void on_object_property_get(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value> &info) {
                        detail::on_property_get<data::ObjectType::WORKER_OBJECT>("on_object_property_get", std::move(name), info);
                    }
                    void
                    on_object_property_set(v8::Local<v8::Name> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value> &info) {
                        detail::on_property_set<data::ObjectType::WORKER_OBJECT>("on_object_property_set", std::move(name), std::move(value), info);
                    }
                    void on_object_property_delete(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Boolean> &info) {
                        detail::on_property_delete<data::ObjectType::WORKER_OBJECT>("on_object_property_delete", std::move(name), info);
                    }
                }
            }
        }
    }
}

#include "estate/internal/server/v8_macro_undef.inl"
