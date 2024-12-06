//
// Created by Scott on 1/13/2022.
//

#include "estate/runtime/enum_op.h"
#include "estate/internal/serenity/worker-process-table.h"
#include "estate/internal/serenity/system/worker-process.h"
#include "estate/internal/deps/boost.h"
#include <cstring>
#include <sys/wait.h>
#include <unordered_set>

using namespace boost::interprocess;

namespace estate {
    bool is_process_alive(pid_t pid) {
        // Wait for child process, this should clean up defunct processes
        waitpid(pid, nullptr, WNOHANG);

        // kill failed let's see why..
        if (kill(pid, 0) == -1) {
            // First of all kill may fail with EPERM if we run as a different user and we have no access, so let's make sure the errno is ESRCH (Process not found!)
            if (errno != ESRCH) {
                return true;
            }
            return false;
        }
        // If kill didn't fail the process is still running
        return true;
    }

    pid_t fork_worker_process_process(WorkerProcessTableS worker_process_table, const u16 worker_process_wait_secs, const WorkerId worker_id,
                                  const WorkerProcessEndpoint &endpoint) {
        mapped_region region(anonymous_shared_memory(sizeof(WorkerProcessListeningLock)));
        auto listening_lock = new(region.get_address()) WorkerProcessListeningLock();

        const auto pid = fork();
        switch (pid) {
            case -1: {
                sys_log_critical("Failed to fork when trying to start WorkerProcess for WorkerId {}", worker_id);
                return -1;
            }
            case 0: {
                auto config = WorkerProcess::LoadConfig(
                        WorkerProcess::SupportedCommand::DeleteWorker |
                        WorkerProcess::SupportedCommand::SetupWorker |
                        WorkerProcess::SupportedCommand::User,
                        worker_id,
                        endpoint.setup_worker_port,
                        endpoint.delete_worker_port,
                        endpoint.user_port);
                WorkerProcessS worker_process = std::make_shared<WorkerProcess>();
                worker_process->init(worker_process_table, config);
                sys_log_trace("Starting worker process");
                worker_process->start();
                listening_lock->is_listening.notify_one();
                sys_log_trace("Notified launcher that worker process has started for worker id {}", worker_id);
                worker_process->run_daemon();
                return 0;
            }
            default: {
                sys_log_trace("Waiting for listening lock to free");
                scoped_lock<boost::interprocess::interprocess_mutex> lock(listening_lock->listening_lock);
                sys_log_trace("Waiting for worker process to start listening");
                if (!listening_lock->is_listening.timed_wait(lock, boost::get_system_time() + boost::posix_time::seconds(worker_process_wait_secs))) {
                    sys_log_critical("Timed out while waiting on new worker process process to start");
                }
                return pid;
            }
        }
    }

    size_t get_worker_process_table_segment_memory_size(size_t num_workers) {
        //TODO: Get this number programmatically somehow.
        //- [sj] History: I started with using sizeof(...) of all the types I was storing in the shared memory.
        //  Then I found out that Boost needs quite a bit more than that. I don't know what for but my
        //  guess is that it's extra management and tracking stuff. It's probably in their docs somewhere.
        static const auto workers_per_page = 50;
        static const auto pages = num_workers / workers_per_page;
        static const auto memory_size = pages * mapped_region::get_page_size();
        return memory_size;
    }

    size_t get_worker_process_ports_segment_memory_size(size_t num_workers) {
        //TODO: Get these numbers programmatically.
        //- [sj] You may be wondering why the ports segment takes, roughly, twice as much memory as the table.
        //  I think it's because the ports segment set will have 3x the values as the table and each item has
        //  far more metadata required than the u16 I'm putting in there.
        static const auto workers_per_page = 25;
        static const auto pages = num_workers / workers_per_page;
        static const auto memory_size = pages * mapped_region::get_page_size();
        return memory_size;
    }

    WorkerProcessTable::WorkerProcessTable(const WorkerProcessTableConfig &config) :
            _launcher_wait_secs{config.launcher_wait_secs}, _worker_process_wait_secs{config.worker_process_wait_secs} {

        sys_log_trace("Removing previous shared memory objects...");

        const auto lock_mem_name{"worker-process-lock"};
        const auto table_mem_name{"worker-process-table"};
        const auto ports_mem_name{"worker-process-ports"};
        shared_memory_object::remove(lock_mem_name);
        shared_memory_object::remove(table_mem_name);
        shared_memory_object::remove(ports_mem_name);

        static const size_t min_shmem_size = 2048;

        static const auto ports_per_worker = 3; //setup_worker, delete_worker, user
        static const auto num_workers = (config.port_end - config.port_start) / ports_per_worker;

        // Create the shared memory for the table

        static const auto table_segment_sz = std::max(get_worker_process_table_segment_memory_size(num_workers), min_shmem_size);
        sys_log_trace("Creating shared memory for the worker process table of size {} bytes", table_segment_sz);
        managed_shared_memory table_segment(create_only, table_mem_name, table_segment_sz);
        TableMapAllocatorT table_alloc(table_segment.get_segment_manager());
        auto table = table_segment.construct<TableMapT>("table")(std::less<WorkerId>(), table_alloc);

        _table_segment = std::move(table_segment);
        _table = table;

#if (false)
        /* This is test code I used to verify I had allocated enough memory in the table segment. */
        for (WorkerId i = 0; i < num_workers; ++i) {
            (*table)[i] = WorkerProcessTableEntry{
                    Code::Ok,
                    WorkerProcessInstance{
                            55,
                            WorkerProcessEndpoint{
                                    1000,
                                    1001,
                                    1002
                            }
                    }
            };
        }
#endif

        // Create the shared memory for the ports set
        static const auto ports_segment_sz = std::max(get_worker_process_ports_segment_memory_size(num_workers), min_shmem_size);
        sys_log_trace("Creating shared memory for the worker process ports of size {} bytes", ports_segment_sz);
        managed_shared_memory ports_segment(create_only, ports_mem_name, ports_segment_sz);
        PortsSetAllocatorT ports_alloc(ports_segment.get_segment_manager());
        auto ports = ports_segment.construct<PortsSetT>("ports")(std::less<u16>(), ports_alloc);

        // add all the ports
        for (u16 p = config.port_start; p <= config.port_end; ++p)
            ports->insert(p);

        _ports_segment = std::move(ports_segment);
        _ports = ports;

        // Create the shared memory for the locks
        sys_log_trace("Creating shared memory for locks...");
        shared_memory_object lock_shm(create_only, lock_mem_name, read_write);
        lock_shm.truncate(sizeof(WorkerProcessTableLock));
        mapped_region lock_region(lock_shm, read_write);
        auto lock_addr = lock_region.get_address();
        auto lock = new(lock_addr) WorkerProcessTableLock();

        _lock_shm = std::move(lock_shm);
        _lock_region = std::move(lock_region);
        _lock = lock;
    }

    // Called from WorkerLoader to get an endpoint
    ResultCode<WorkerProcessEndpoint> WorkerProcessTable::loader_get_endpoint(const LogContext &log_context, WorkerId worker_id) {
        using Result = ResultCode<WorkerProcessEndpoint>;

        {
            log_trace(log_context, "Waiting for changes lock");
            scoped_lock<boost::interprocess::interprocess_mutex> lock(_lock->changes_lock);

            // Get the endpoint if it exists and is still running, otherwise add an empty entry and signal the launcher to update.
            auto &table = *_table;
            const auto it = table.find(worker_id);
            if (it == table.end()) {
                // Does not exist, add new so it can be filled in
                table[worker_id] = std::move(WorkerProcessTableEntry{});
                log_trace(log_context, "The WorkerId doesn't exist in the worker process table, added and notifying launcher changes exist");
                _lock->has_changes = true;
            } else {
                auto &entry = it->second;
                if (entry.deleted) {
                    return Result::Error(Code::WorkerProcess_WorkerDeleted);
                }
                if (entry.exists()) {
                    auto &instance = entry.instance.value();
                    if (it->second.is_running()) {
                        // OK, return endpoint
                        auto &endpoint = instance.endpoint;
                        log_trace(log_context,
                                  "(pre-check) Successfully retrieved existing endpoint for WorkerId {}, setup_worker_port {}, delete_worker_port {}, user_port {}",
                                  worker_id, endpoint.setup_worker_port, endpoint.delete_worker_port, endpoint.user_port);
                        return Result::Ok(endpoint);
                    } else {
                        log_trace(log_context, "Reclaiming ports for worker process table entry because the process is no longer running");

                        //reclaim the ports since the process is dead
                        _ports->insert(instance.endpoint.setup_worker_port);
                        _ports->insert(instance.endpoint.delete_worker_port);
                        _ports->insert(instance.endpoint.user_port);

                        // Reset the entry so the launcher knows to start it anew.
                        table[worker_id] = std::move(WorkerProcessTableEntry{});
                        _lock->has_changes = true;
                    }
                }
            }
        }

        {
            log_trace(log_context, "Waiting for launcher to signal it has updated the worker process process table");

            scoped_lock<boost::interprocess::interprocess_mutex> lock(_lock->updated_lock);

            // Wait for the Launcher to fork the new WorkerProcess
            if (!_lock->wake_has_updated.timed_wait(lock, boost::get_system_time() + boost::posix_time::seconds(_launcher_wait_secs))) {
                log_error(log_context, "Timed out while waiting for the launcher to spawn worker process for worker id {}", worker_id);
                return Result::Error(Code::Launcher_TimedOutWhileGettingWorkerProcess);
            }

            log_trace(log_context, "Successfully waited for launcher");
        }

        {
            log_trace(log_context, "Waiting for changes lock to get the launcher's updates to the worker process table");

            scoped_lock<boost::interprocess::interprocess_mutex> lock(_lock->changes_lock);

            const auto it = _table->find(worker_id);
            if (it != _table->end()) {
                if (it->second.exists() && !it->second.deleted) {
                    //Found = Ok
                    assert(it->second.instance.has_value());
                    auto &endpoint = it->second.instance.value().endpoint;
                    log_trace(log_context,
                              "Successfully retrieved existing endpoint for WorkerId {}, setup_worker_port {}, delete_worker_port {}, user_port {}",
                              worker_id, endpoint.setup_worker_port, endpoint.delete_worker_port, endpoint.user_port);
                    return Result::Ok(endpoint);
                } else {
                    log_error(log_context,
                              "Launcher failed to spawn worker process, see launcher logs for error. The worker process entry did not exist when it was expected to.");
                    return Result::Error(Code::Launcher_FailedToSpawnWorkerProcess);
                }
            }

            log_error(log_context, "Launcher failed to spawn worker process, see launcher logs for error");
            return Result::Error(Code::Launcher_FailedToSpawnWorkerProcess);
        }
    }
    // Called from the Launcher process
    LauncherUpdateResult WorkerProcessTable::launcher_update() {
        auto changes_made{false};

        {
            scoped_lock<boost::interprocess::interprocess_mutex> lock(_lock->changes_lock);

            waitpid(0, nullptr, WNOHANG); //reclaim zombie processes

            if(!_lock->has_changes) {
                return LauncherUpdateResult::IDLE;
            }

            _lock->has_changes = false;

            sys_log_trace("Found that changes have been made, searching for updates");

            std::unordered_set<WorkerId> waiting_to_be_deleted{};
            std::unordered_set<WorkerId> to_be_removed{};

            do {
                if (to_be_removed.size() > 0) {
                    for (const auto worker_id: to_be_removed) {
                        sys_log_trace("Removed worker id {} from the worker process process table", worker_id);
                        _table->erase(worker_id);
                    }
                    to_be_removed.clear();
                    changes_made = true;
                }

                // find all the entries that need updating
                for (auto &item: *_table) {
                    auto worker_id = item.first;
                    auto &entry = item.second;

                    if (entry.deleted) {
                        sys_log_trace("Found that worker id {} entry was marked deleted.", worker_id);

                        // if it's been deleted and it's no longer running, reclaim the ports
                        if (!entry.is_running()) /*NOTE: will refresh the Linux Kernel process table */ {
                            sys_log_trace("Found that worker id {} worker process process is no longer running.", worker_id);
                            waiting_to_be_deleted.erase(worker_id);
                            if (entry.instance.has_value()) {
                                const auto &endpoint = entry.instance.value().endpoint;
                                sys_log_trace("(deleted) Reclaiming ports {},{}, and {}", endpoint.setup_worker_port, endpoint.delete_worker_port,
                                              endpoint.user_port);
                                //Reclaim the ports since the instance is no longer running
                                _ports->insert(endpoint.setup_worker_port);
                                _ports->insert(endpoint.delete_worker_port);
                                _ports->insert(endpoint.user_port);
                                changes_made = true;
                            }
                            to_be_removed.insert(worker_id);
                            break;
                        } else {
                            sys_log_trace("Waiting for worker id {} worker process process to stop", worker_id);
                            waiting_to_be_deleted.insert(worker_id);
                            continue;
                        }
                    }

                    if (entry.is_running())
                        continue; //can't do anything when it's running, must wait for it to stop on its own

                    sys_log_trace("Found non-running entry in worker process process table for worker id {}", worker_id);

                    if (entry.instance.has_value()) {
                        //Reclaim the ports since the instance is no longer running
                        const auto &endpoint = entry.instance.value().endpoint;

                        sys_log_trace("(stopped) Reclaiming ports {},{}, and {}", endpoint.setup_worker_port, endpoint.delete_worker_port,
                                      endpoint.user_port);

                        _ports->insert(endpoint.setup_worker_port);
                        _ports->insert(endpoint.delete_worker_port);
                        _ports->insert(endpoint.user_port);

                        entry.instance = std::nullopt;
                        changes_made = true;
                    }


                    if (_ports->size() < 3) {
                        sys_log_critical("Not enough free ports to launch worker process");
                        return LauncherUpdateResult::FAILURE;
                    }

                    const auto setup_worker_port = *_ports->begin();
                    _ports->erase(_ports->begin());

                    const auto delete_worker_port = *_ports->begin();
                    _ports->erase(_ports->begin());

                    const auto user_port = *_ports->begin();
                    _ports->erase(_ports->begin());

                    WorkerProcessEndpoint endpoint{
                            setup_worker_port,
                            delete_worker_port,
                            user_port
                    };

                    sys_log_trace("Assigning ports {},{}, and {} to worker id {}", endpoint.setup_worker_port, endpoint.delete_worker_port,
                                  endpoint.user_port, worker_id);

                    const auto pid = fork_worker_process_process(this->shared_from_this(), this->_worker_process_wait_secs, worker_id, endpoint);
                    switch (pid) {
                        case 0: //in WorkerProcess process
                        case -1: //Failed to fork, already logged
                            return LauncherUpdateResult::FAILURE;
                        default:
                            sys_log_trace("Forked worker process process {} for worker id {}", pid, worker_id);
                            break;
                    }

                    WorkerProcessInstance instance{
                            pid,
                            std::move(endpoint)
                    };

                    entry.instance = std::move(instance);
                    changes_made = true;
                }

                if (waiting_to_be_deleted.size() > 0) {
                    sys_log_trace("Waiting for one or more processes to be deleted");
                    sleep(1);
                }
            } while (waiting_to_be_deleted.size() > 0 || to_be_removed.size() > 0);
        }

        if (changes_made) {
            sys_log_trace("Notified that the launcher has made updates to the worker process process table");
            _lock->wake_has_updated.notify_one();
        }

        return LauncherUpdateResult::CONTINUE;
    }
    // Called from WorkerProcess to mark it as deleted.
    void WorkerProcessTable::mark_worker_process_deleted(const LogContext &log_context, WorkerId worker_id) {
        log_trace(log_context, "Waiting for changes lock to mark worker process deleted");
        scoped_lock<boost::interprocess::interprocess_mutex> lock(_lock->changes_lock);
        auto &table = *_table;
        const auto it = table.find(worker_id);
        if (it == table.end()) {
            // Doesn't exist so add an entry saying it does but has been deleted.
            table[worker_id] = std::move(WorkerProcessTableEntry{true, std::nullopt});
            // this shouldn't ever happen since this is called from the process in question
            log_warn(log_context, "The WorkerId didn't exist in the worker process table even though it's being marked deleted from the process itself.");
            //[sic] Since this worker has been newly deleted there's no reason to update the launcher.
        } else {
            auto &entry = it->second;
            if (!entry.deleted) {
                entry.deleted = true;
                _lock->has_changes = true;
                log_trace(log_context, "WorkerProcess marked as deleted");
            }
        }
    }
    bool WorkerProcessTableEntry::is_running() const {
        return exists() && is_process_alive(instance.value().pid);
    }
    bool WorkerProcessTableEntry::exists() const {
        return instance.has_value() && instance.value().pid > 0;
    }
}