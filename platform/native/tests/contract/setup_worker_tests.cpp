#include "../estate_test.h"
#include "gtest/gtest.h"

#include <estate/internal/deps/boost.h>

using namespace estate;

TEST(contract_setup_worker_tests, CanSetupNewWorker) {
    auto services = test::setup_serenity_processors(make_test_log_context, 5002, false, true, false).unwrap();
    auto log_context = make_test_log_context;
    std::cout << boost::filesystem::current_path() << std::endl;
    test::util::setup_worker_from_directory(log_context, services, test_data_dir, 0);
}

TEST(contract_setup_worker_tests, CanSetupExistingWorker) {
    auto services = test::setup_serenity_processors(make_test_log_context, 6001, false, true, false).unwrap();

    auto log_context = make_test_log_context;

    auto test_data_dir_fmt = test_data_dir;
    test_data_dir_fmt.append("/{0}");

    test::util::setup_worker_from_directory(log_context, services,
                                          fmt::format(test_data_dir_fmt, 5000),
                                          0);

    test::util::setup_worker_from_directory(log_context, services,
                                          fmt::format(test_data_dir_fmt, 5005),
                                          5000);
}
