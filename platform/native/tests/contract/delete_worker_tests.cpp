#include <estate/internal/deps/boost.h>

#include "../estate_test.h"
#include "gtest/gtest.h"

using namespace estate;

TEST(contract_delete_worker_tests, CantDeleteWorkerThatDoesntExist) {
    auto services = test::setup_serenity_processors(make_test_log_context, 1, false, true, true).unwrap();
    auto log_context = make_test_log_context;

    fbs::Builder builder;
    auto req = test::create_delete_worker_request(log_context, builder, 1, 1);
    services->delete_worker->processor->post(std::move(req), services->delete_worker->request_context_wrapper->create_request_context());
    services->delete_worker->request_context_wrapper->wait(log_context);

    const auto *response = services->delete_worker->request_context_wrapper->get_result().get_payload();
    ASSERT_TRUE(response->error());
    ASSERT_EQ(response->error()->error_code(), GET_CODE_VALUE(Code::Datastore_UnableToOpen));
}

TEST(contract_delete_worker_tests, CanDeleteWorker) {
    auto services = test::setup_serenity_processors(make_test_log_context, 1, false, true, true).unwrap();
    auto log_context = make_test_log_context;
    auto package = test::util::setup_worker_from_directory(log_context, services, test_data_dir, 0);

    ASSERT_TRUE(services->database_manager->get_database(log_context, package->worker_id, false, std::nullopt));

    fbs::Builder builder;
    auto req = test::create_delete_worker_request(log_context, builder, package->worker_id, package->worker_version);
    services->delete_worker->processor->post(std::move(req), services->delete_worker->request_context_wrapper->create_request_context());
    services->delete_worker->request_context_wrapper->wait(log_context);

    const auto *response = services->delete_worker->request_context_wrapper->get_result().get_payload();
    ASSERT_FALSE(response->error());

    //Make sure the deleted file prevents the database from being opened
    ASSERT_EQ(services->database_manager->get_database(log_context, package->worker_id, false, std::nullopt).get_error(), Code::Datastore_DeletedFileExists);
    ASSERT_EQ(services->database_manager->get_database(log_context, package->worker_id, true, std::make_optional(1)).get_error(), Code::Datastore_DeletedFileExists);

    auto deleted_file = fmt::format(services->database_manager->get_config().deleted_file_format, package->worker_id);
    ASSERT_TRUE(boost::filesystem::remove(deleted_file));

    //Make sure the internal deleted flag prevents the database from being opened
    ASSERT_EQ(services->database_manager->get_database(log_context, package->worker_id, false, std::nullopt).get_error(), Code::Datastore_DeletedFlagExists);
    ASSERT_EQ(services->database_manager->get_database(log_context, package->worker_id, true, std::make_optional(1)).get_error(), Code::Datastore_DeletedFlagExists);
}
