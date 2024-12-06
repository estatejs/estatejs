//
// Originally written by Scott R. Jones.
// Copyright (c) 2021 Warpdrive Technologies, Inc. All rights reserved.
//

#pragma once

#include "estate/internal/outerspace/outerspace-fwd.h"
#include "estate/runtime/protocol/model_generated.h"

#include <estate/runtime/model_types.h>

#include <estate/internal/deps/boost.h>

#include <cassert>
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <memory>

namespace estate::outerspace {
    struct DataUpdateSubscription {
        ClassId class_id;
        std::string primary_key_str;
        struct Hasher {
            std::size_t operator()(const DataUpdateSubscription &sub) const;
        };
    };
    bool operator==(const DataUpdateSubscription &lhs, const DataUpdateSubscription &rhs);
    template<typename T>
    void get_event_source_info(const T& r, ClassId& class_id, std::string& primary_key) {
        switch(r->source_type()) {
            case WorkerReferenceUnionProto::DataReferenceValueProto: {
                    const auto ref = r->source_as_DataReferenceValueProto();
                    class_id = ref->class_id();
                    primary_key = ref->primary_key()->str();
                }
                break;
            case WorkerReferenceUnionProto::ServiceReferenceValueProto: {
                    const auto ref = r->source_as_ServiceReferenceValueProto();
                    class_id = ref->class_id();
                    primary_key = ref->primary_key()->str();
                }
                break;
            default:
                assert(false);
        }
    }
    struct MessageSubscription {
        ClassId event_class_id;
        ClassId source_class_id;
        std::string source_primary_key_str;
        struct Hasher {
            std::size_t operator()(const MessageSubscription &sub) const;
        };
        template<typename RT>
        static MessageSubscription CreateFrom(const RT& request) {
            ClassId source_class_id;
            std::string source_primary_key;
            get_event_source_info(request, source_class_id, source_primary_key);
            return MessageSubscription {
                    request->class_id(),
                    source_class_id,
                    std::move(source_primary_key)
            };
        }
    };
    bool operator==(const MessageSubscription &lhs, const MessageSubscription &rhs);

    template<typename S, typename H = typename S::Hasher>
    class SubscriptionMappings {
        std::mutex _mutex{};
        std::unordered_map<S, std::unordered_set<SessionHandle>, H> _subscriptions{};
        std::unordered_map<SessionHandle, std::unordered_set<const S *>> _session_handles{};
    public:
        SubscriptionMappings() = default;
        SubscriptionMappings(const SubscriptionMappings &) = delete;
        SubscriptionMappings(SubscriptionMappings &&move_from) = delete;
        void subscribe(SessionHandle session_handle, S subscription) {
            std::scoped_lock<std::mutex> lck{_mutex};
            auto sub_it = _subscriptions.find(subscription);
            const S *sub;
            if (sub_it == _subscriptions.end()) {
                const auto[at, inserted] = _subscriptions.emplace(
                        std::make_pair(
                                std::move(subscription),
                                std::unordered_set<SessionHandle>({session_handle})
                        ));
                assert(inserted);
                sub = &at->first;
            } else {
                sub = &sub_it->first;
                sub_it->second.emplace(session_handle);
            }
            auto sess_it = _session_handles.find(session_handle);
            if (sess_it == _session_handles.end()) {
                const auto[_, inserted] = _session_handles.emplace(std::make_pair(session_handle, std::unordered_set<const S *>({sub})));
                assert(inserted);
            } else {
                sess_it->second.emplace(sub);
            }
        }
        void unsubscribe_all(SessionHandle session_handle) {
            std::scoped_lock<std::mutex> lck{_mutex};

            const auto sess_it = _session_handles.find(session_handle);
            if (sess_it == _session_handles.end()) {
                return; //not subscribed to anything
            }

            for (const S *s : sess_it->second) {
                const auto sub_it = _subscriptions.find(*s);
                assert(sub_it != _subscriptions.end());
                {
                    const auto count = sub_it->second.erase(session_handle);
                    assert(count == 1);
                }
                if (sub_it->second.empty()) {
                    const auto count = _subscriptions.erase(*s);
                    assert(count == 1);
                }
            }
            const auto count = _session_handles.erase(session_handle);
            assert(count == 1);
        }
        void unsubscribe(SessionHandle session_handle, S subscription) {
            std::scoped_lock<std::mutex> lck{_mutex};
            const auto sub_it = _subscriptions.find(subscription);
            if (sub_it == _subscriptions.end())
                return;

            {
                const auto count = sub_it->second.erase(session_handle);
                assert(count == 1);
            }

            const auto sess_it = _session_handles.find(session_handle);
            {
                const auto count = sess_it->second.erase(&sub_it->first);
                assert(count == 1);
            }

            if (sub_it->second.empty()) {
                const auto count = _subscriptions.erase(subscription);
                assert(count == 1);
            }

            if (sess_it->second.empty()) {
                const auto count = _session_handles.erase(session_handle);
                assert(count == 1);
            }
        }
        std::optional<std::unordered_set<SessionHandle>> get_subscribed_sessions(const S &subscription) {
            const auto it = _subscriptions.find(subscription);
            if (it != _subscriptions.end())
                return it->second;
            return std::nullopt;
        }
    };
    template<typename S, typename H = typename S::Hasher>
    using SubscriptionMappingsU = std::unique_ptr<SubscriptionMappings<S, H>>;

    //TODO: move this into Redis (etc.) when we need multiple Rivers.
    class SubscriptionManager {
        std::mutex _worker_id_object_update_mappings_mutex{};
        std::unordered_map<WorkerId, SubscriptionMappingsU<DataUpdateSubscription>> _worker_id_object_update_mappings{};
        std::mutex _worker_id_event_mappings_mutex{};
        std::unordered_map<WorkerId, SubscriptionMappingsU<MessageSubscription>> _worker_id_event_mappings{};
    public:
        SubscriptionManager() = default;
        SubscriptionManager(const SubscriptionManager &) = delete;
        SubscriptionManager(SubscriptionManager &&) = delete;
        void unsubscribe_all(WorkerId worker_id, SessionHandle sesson_handle);
        void subscribe(WorkerId worker_id, SessionHandle session_handle, DataUpdateSubscription subscription);
        void subscribe(WorkerId worker_id, SessionHandle session_handle, MessageSubscription subscription);
        void unsubscribe(WorkerId worker_id, SessionHandle session_handle, DataUpdateSubscription subscription);
        void unsubscribe(WorkerId worker_id, SessionHandle session_handle, MessageSubscription subscription);
        std::optional<std::unordered_set<SessionHandle>> maybe_get_subscribers(WorkerId worker_id, const DataUpdateSubscription &subscription);
        std::optional<std::unordered_set<SessionHandle>> maybe_get_subscribers(WorkerId worker_id, const MessageSubscription &subscription);
    };
    using SubscriptionManagerS = std::shared_ptr<SubscriptionManager>;
}
