//
// Originally written by Scott R. Jones.
// Copyright (c) 2021 Warpdrive Technologies, Inc. All rights reserved.
//

#include "estate/internal/outerspace/subscription.h"

#include <estate/internal/logging.h>

namespace estate::outerspace {
    std::size_t DataUpdateSubscription::Hasher::operator()(const DataUpdateSubscription &sub) const {
        using boost::hash_value;
        using boost::hash_combine;
        std::size_t seed = 0;
        hash_combine(seed, hash_value(sub.class_id));
        hash_combine(seed, hash_value(sub.primary_key_str));
        return seed;
    }
    std::size_t MessageSubscription::Hasher::operator()(const MessageSubscription &sub) const {
        using boost::hash_value;
        using boost::hash_combine;
        std::size_t seed = 0;
        hash_combine(seed, hash_value(sub.event_class_id));
        hash_combine(seed, hash_value(sub.source_class_id));
        hash_combine(seed, hash_value(sub.source_primary_key_str));
        return seed;
    }
    void SubscriptionManager::unsubscribe_all(WorkerId worker_id, SessionHandle sesson_handle) {
        {
            std::scoped_lock<std::mutex> lck{_worker_id_object_update_mappings_mutex};
            const auto it = _worker_id_object_update_mappings.find(worker_id);
            if (it != _worker_id_object_update_mappings.end()) {
                it->second->unsubscribe_all(sesson_handle);
            }
        }
        {
            std::scoped_lock<std::mutex> lck{_worker_id_event_mappings_mutex};
            const auto it = _worker_id_event_mappings.find(worker_id);
            if (it != _worker_id_event_mappings.end()) {
                it->second->unsubscribe_all(sesson_handle);
            }
        }
    }
    void SubscriptionManager::subscribe(WorkerId worker_id, SessionHandle session_handle, DataUpdateSubscription subscription) {
        std::scoped_lock<std::mutex> lck{_worker_id_object_update_mappings_mutex};
        const auto it = _worker_id_object_update_mappings.find(worker_id);
        if (it == _worker_id_object_update_mappings.cend()) {
            auto mappings = std::make_unique<SubscriptionMappings<DataUpdateSubscription>>();
            mappings->subscribe(session_handle, std::move(subscription));
            _worker_id_object_update_mappings.emplace(std::make_pair(worker_id, std::move(mappings)));
        } else {
            it->second->subscribe(session_handle, std::move(subscription));
        }
    }
    void SubscriptionManager::subscribe(WorkerId worker_id, SessionHandle session_handle, MessageSubscription subscription) {
        std::scoped_lock<std::mutex> lck{_worker_id_event_mappings_mutex};
        const auto it = _worker_id_event_mappings.find(worker_id);
        if (it == _worker_id_event_mappings.cend()) {
            auto mappings = std::make_unique<SubscriptionMappings<MessageSubscription>>();
            mappings->subscribe(session_handle, std::move(subscription));
            _worker_id_event_mappings.emplace(std::make_pair(worker_id, std::move(mappings)));
        } else {
            it->second->subscribe(session_handle, std::move(subscription));
        }
    }
    void SubscriptionManager::unsubscribe(WorkerId worker_id, SessionHandle session_handle, DataUpdateSubscription subscription) {
        std::scoped_lock<std::mutex> lck{_worker_id_object_update_mappings_mutex};
        const auto it = _worker_id_object_update_mappings.find(worker_id);
        if (it != _worker_id_object_update_mappings.cend()) {
            it->second->unsubscribe(session_handle, std::move(subscription));
        }
    }
    void SubscriptionManager::unsubscribe(WorkerId worker_id, SessionHandle session_handle, MessageSubscription subscription) {
        std::scoped_lock<std::mutex> lck{_worker_id_event_mappings_mutex};
        const auto it = _worker_id_event_mappings.find(worker_id);
        if (it != _worker_id_event_mappings.cend()) {
            it->second->unsubscribe(session_handle, std::move(subscription));
        }
    }
    std::optional<std::unordered_set<SessionHandle>> SubscriptionManager::maybe_get_subscribers(WorkerId worker_id, const DataUpdateSubscription &subscription) {
        std::scoped_lock<std::mutex> lck{_worker_id_object_update_mappings_mutex};
        const auto it = _worker_id_object_update_mappings.find(worker_id);
        if (it == _worker_id_object_update_mappings.cend())
            return std::nullopt;
        return it->second->get_subscribed_sessions(subscription);
    }
    std::optional<std::unordered_set<SessionHandle>> SubscriptionManager::maybe_get_subscribers(WorkerId worker_id, const MessageSubscription &subscription) {
        std::scoped_lock<std::mutex> lck{_worker_id_event_mappings_mutex};
        const auto it = _worker_id_event_mappings.find(worker_id);
        if (it == _worker_id_event_mappings.cend())
            return std::nullopt;
        return it->second->get_subscribed_sessions(subscription);
    }
    bool operator==(const DataUpdateSubscription &lhs, const DataUpdateSubscription &rhs) {
        return lhs.class_id == rhs.class_id && lhs.primary_key_str == rhs.primary_key_str;
    }
    bool operator==(const MessageSubscription &lhs, const MessageSubscription &rhs) {
        return lhs.event_class_id == rhs.event_class_id &&
        lhs.source_class_id == rhs.source_class_id &&
        lhs.source_primary_key_str == rhs.source_primary_key_str;
    }
}
