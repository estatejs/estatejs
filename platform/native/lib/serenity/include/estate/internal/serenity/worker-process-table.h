//
// Created by Scott on 1/13/2022.
//

#pragma once

#include "estate/runtime/model_types.h"
#include "estate/runtime/code.h"
#include "estate/internal/deps/boost.h"
#include "estate/internal/local_config.h"
#include "estate/internal/logging.h"

#include <optional>

namespace estate {
    bool is_process_alive(pid_t pid);
    struct WorkerProcessEndpoint {
        u16 setup_worker_port;
        u16 delete_worker_port;
        u16 user_port;
    };
    struct WorkerProcessInstance {
        pid_t pid;
        WorkerProcessEndpoint endpoint;
    };
    struct WorkerProcessTableEntry {
        bool deleted;
        std::optional<WorkerProcessInstance> instance;
        bool exists() const;
        bool is_running() const;
    };
    struct WorkerProcessTableConfig {
        u16 launcher_wait_secs;
        u16 worker_process_wait_secs;
        u16 port_start;
        u16 port_end;
        static WorkerProcessTableConfig FromRemote(const LocalConfigurationReader &reader) {
            return WorkerProcessTableConfig{
                    reader.get_u16("launcher_wait_secs"),
                    reader.get_u16("worker_process_wait_secs"),
                    reader.get_u16("port_start"),
                    reader.get_u16("port_end"),
            };
        }
    };
    struct WorkerProcessTableLock {
        bool has_changes{false};
        boost::interprocess::interprocess_mutex changes_lock;
        boost::interprocess::interprocess_mutex updated_lock;
        boost::interprocess::interprocess_condition wake_has_updated;
    };
    struct WorkerProcessListeningLock {
        boost::interprocess::interprocess_mutex listening_lock;
        boost::interprocess::interprocess_condition is_listening;
    };
    enum class LauncherUpdateResult : u8 {
        IDLE = 0,
        CONTINUE = 1,
        FAILURE = 2
    };
    struct WorkerProcessTable : public std::enable_shared_from_this<WorkerProcessTable> {
        typedef std::pair<const WorkerId, WorkerProcessTableEntry> ValueT;
        typedef boost::interprocess::allocator<ValueT, boost::interprocess::managed_shared_memory::segment_manager> TableMapAllocatorT;
        typedef boost::interprocess::map<WorkerId, WorkerProcessTableEntry, std::less<WorkerId>, TableMapAllocatorT> TableMapT;
        typedef boost::interprocess::allocator<u16, boost::interprocess::managed_shared_memory::segment_manager> PortsSetAllocatorT;
        typedef boost::interprocess::set<u16, std::less<u16>, PortsSetAllocatorT> PortsSetT;
        WorkerProcessTable(const WorkerProcessTableConfig &config);
        ~WorkerProcessTable() = default;
        WorkerProcessTable(const WorkerProcessTable &) = delete; //no copy
        WorkerProcessTable(WorkerProcessTable &&) = delete; //no move
        void mark_worker_process_deleted(const LogContext &log_context, WorkerId worker_id);
        ResultCode <WorkerProcessEndpoint> loader_get_endpoint(const LogContext &log_context, WorkerId worker_id);
        LauncherUpdateResult launcher_update();
    private:
        boost::interprocess::shared_memory_object _lock_shm;
        boost::interprocess::mapped_region _lock_region;
        boost::interprocess::managed_shared_memory _table_segment;
        boost::interprocess::managed_shared_memory _ports_segment;
        TableMapT *_table;
        PortsSetT *_ports;
        WorkerProcessTableLock *_lock;
        const u16 _launcher_wait_secs;
        const u16 _worker_process_wait_secs;
    };
    using WorkerProcessTableS = std::shared_ptr<WorkerProcessTable>;
}