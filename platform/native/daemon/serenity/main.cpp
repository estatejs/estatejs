#include <iostream>
#include "estate/internal/serenity/system/launcher.h"
#include "estate/internal/local_config.h"
#include <proc/readproc.h>
#include <unordered_set>

using namespace estate;

std::string get_process_name(const char *my_path) {
    boost::filesystem::path p(my_path);
    return p.filename().string();
}

std::unordered_set<int> get_other_processes(const std::string &my_process_name) {
    PROCTAB *proc = openproc(PROC_FILLMEM | PROC_FILLSTAT | PROC_FILLSTATUS);

    proc_t proc_info;
    memset(&proc_info, 0, sizeof(proc_info));
    std::unordered_set<int> pids{};

    const auto my_pid = ::getpid();
    while (readproc(proc, &proc_info) != NULL) {
        if (proc_info.tid != my_pid && std::strcmp(proc_info.cmd, my_process_name.c_str()) == 0) {
            pids.insert(proc_info.tid);
        }
    }

    closeproc(proc);

    return std::move(pids);
}

int main(int argc, const char **argv) {
    if (argc != 1) {
        std::cerr << "Fatal: No arguments allowed." << std::endl;
        return 1;
    }

    // Ensure that this is the only serenity process running.
    const auto my_process_name = get_process_name(argv[0]);
    if (get_other_processes(my_process_name).size() != 0) {
        std::cerr << "Other " << my_process_name << " processes are running. Either terminate the other processes or reboot." << std::endl;
        return 1;
    }

    const auto config = Launcher::load_config();
    Launcher launcher{};
    launcher.init(config);
    launcher.run();
}
