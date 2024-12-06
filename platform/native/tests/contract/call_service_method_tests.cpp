#include "../estate_test.h"

#include <gtest/gtest.h>

#include "../val_def.h"

using namespace estate;

TEST(contract_call_service_method_tests, ServerApi) {
    const auto d = test_data_dir;
    SETUP(1, true, true, false);

    PrimaryKey service_primary_key{std::string{"default"}};
    PrimaryKey associated_data_primary_key{std::string{"ScottJones"}};
    const auto uuid_len = 36;
    const auto uuid_len_no_dashes = 32;

    int c = 1;
    ClassId data_class_id = c++;
    ClassId associated_data_class_id = c++;
    ClassId service_class_id = c++;
    ClassId data_added_class_id = c++;
    ClassId data_deleted_class_id = c++;
    ClassId metadata_service_class_id = c++;

    int m = 100;
    MethodId method_getObjectTest = m++;
    MethodId method_getObjectNotFoundTest = m++;
    MethodId method_saveAndEventsTest = m++;
    MethodId method_argDeltaDoesntCauseUnsavedDataWhenPreviouslySaved1 = m++;
    MethodId method_argDeltaDoesntCauseUnsavedDataWhenPreviouslySaved2 = m++;
    MethodId method_dataExists = m++;
    MethodId method_revertObjectToNothingTest = m++;
    MethodId method_revertObjectToSaveTest = m++;
    MethodId method_unsavedChangesTest = m++;
    MethodId method_revertSelfTest = m++;
    MethodId method_createServiceTest = m++;
    MethodId method_getServiceTest = m++;
    MethodId method_invalidCallTest = m++;
    MethodId method_inboundDeltaCreateObject = m++;
    MethodId method_inboundDeltaAddProperty = m++;
    MethodId method_inboundDeltaDeleteProperty = m++;
    MethodId method_inboundDeltaNoChanges = m++;
    MethodId method_deleteSelfTest = m++;
    MethodId method_deleteSelfTestConfirm = m++;
    MethodId method_deleteTest1 = m++;
    MethodId method_deleteTest2 = m++;
    MethodId method_deleteTest3 = m++;
    MethodId method_purgeTest1 = m++;
    MethodId method_purgeTest2 = m++;
    MethodId method_purgeTest3 = m++;
    MethodId method_evalTest = m++;

    SUBTEST_BEGIN(Arg Delta Doesnt Cause Unsaved Data When Previously Saved)
    {
        //(1) call to save and get the handle + delta
        std::string data_primary_key;
        {
            const auto response = context.call_service_method(service_class_id, service_primary_key,
                                                              method_argDeltaDoesntCauseUnsavedDataWhenPreviouslySaved1,
                                                              std::nullopt, std::nullopt);
            ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

            { // Validate the console log
                ASSERT_TRUE(response->console_log());
                const auto messages = response->console_log_nested_root()->messages();
                ASSERT_TRUE(messages && messages->size() == 4);
                ASSERT_FALSE(messages->Get(0)->error());
                ASSERT_EQ(messages->Get(0)->message()->str(), "Log 1");
                ASSERT_TRUE(messages->Get(1)->error());
                ASSERT_EQ(messages->Get(1)->message()->str(), "Error 1");
                ASSERT_FALSE(messages->Get(2)->error());
                ASSERT_EQ(messages->Get(2)->message()->str(), "Log 2");
                ASSERT_TRUE(messages->Get(3)->error());
                ASSERT_EQ(messages->Get(3)->message()->str(), "Error 2");
            }

            { // Validate the response and get the data pk
                const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
                const auto return_value = inner_response->return_value();
                ASSERT_EQ(return_value->value_type(), ValueUnionProto::DataReferenceValueProto);
                const auto data_ref = return_value->value_as_DataReferenceValueProto();
                ASSERT_EQ(data_ref->class_id(), data_class_id);
                data_primary_key = data_ref->primary_key()->str();
                ASSERT_EQ(data_primary_key.size(), uuid_len);
            }

            ASSERT_TRUE(response->deltas());
            ASSERT_EQ(response->deltas()->size(), 1);

            { // Delta
                const auto delta = response->deltas()->Get(0)->bytes_nested_root();
                ASSERT_FALSE(delta->deleted());
                ASSERT_EQ(delta->class_id(), data_class_id);
                ASSERT_EQ(delta->object_version(), 1);
                ASSERT_EQ(delta->primary_key()->str(), data_primary_key);
                ASSERT_FALSE(delta->deleted_properties());
                ASSERT_TRUE(delta->properties());
                const auto properties = delta->properties();
                ASSERT_EQ(properties->size(), 1);
                const auto p = properties->Get(0);
                ASSERT_EQ(p->name()->str(), "name");
                ASSERT_EQ(p->value_bytes_nested_root()->value_as_StringValueProto()->value()->str(), "something something whatever nothing");
            }
        }

        // (2) pass the ref and the delta for it as an argument and return it
        {
            std::vector<fbs::Offset<ValueProto>> arguments{};
            SET_BUILDER(context.builder);
            arguments.push_back(WOBJ_VAL(data_class_id, PrimaryKey{data_primary_key}));

            std::vector<fbs::Offset<InboundDataDeltaProto>> referenced_data_deltas{};
            {
                std::vector<fbs::Offset<NestedPropertyProto>> properties{};

                fbs::Builder inner_builder{};
                inner_builder.Finish(CreateValueProto(inner_builder, ValueUnionProto::StringValueProto,
                                                      CreateStringValueProto(inner_builder, inner_builder.CreateString(
                                                              "something something whatever nothing")).Union()));
                properties.push_back(CreateNestedPropertyProto(context.builder, context.builder.CreateString("name"),
                                                               context.builder.CreateVector(inner_builder.GetBufferPointer(),
                                                                                            inner_builder.GetSize())));
                referenced_data_deltas.push_back(
                        CreateInboundDataDeltaProto(context.builder,
                                                          data_class_id,
                                                          1,
                                                          context.builder.CreateString(data_primary_key),
                                                          context.builder.CreateVector(properties),
                                                          0));
            }

            const auto response = context.call_service_method(service_class_id, service_primary_key,
                                                              method_argDeltaDoesntCauseUnsavedDataWhenPreviouslySaved2,
                                                              std::move(arguments), referenced_data_deltas);
            ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

            { // Validate the response and get the data pk
                const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
                const auto return_value = inner_response->return_value();
                ASSERT_EQ(return_value->value_type(), ValueUnionProto::DataReferenceValueProto);
                const auto data_ref = return_value->value_as_DataReferenceValueProto();
                ASSERT_EQ(data_ref->class_id(), data_class_id);
                ASSERT_EQ(data_ref->primary_key()->str(), data_primary_key);
            }

            ASSERT_FALSE(response->deltas());
            ASSERT_TRUE(response->events());
            ASSERT_EQ(response->events()->size(), 1);

            { // First event
                const auto data_added = response->events()->Get(0)->bytes_nested_root();

                //validate the source
                ASSERT_EQ(WorkerReferenceUnionProto::ServiceReferenceValueProto, data_added->source_type());
                const auto ref = data_added->source_as_ServiceReferenceValueProto();
                ASSERT_EQ(service_class_id, ref->class_id());
                ASSERT_EQ(service_primary_key.view(), ref->primary_key()->string_view());

                ASSERT_EQ(data_added->class_id(), data_added_class_id);

                { //Validate the DataAdded event's properties
                    ASSERT_EQ(data_added->properties()->size(), 1);
                    const auto p = data_added->properties()->Get(0);
                    ASSERT_EQ(p->name()->str(), "data");
                    const auto data_ref = p->value()->value_as_DataReferenceValueProto();
                    ASSERT_EQ(data_ref->class_id(), data_class_id);
                    ASSERT_EQ(data_ref->primary_key()->str(), data_primary_key);
                }

                { //Validate the referenced worker object handles
                    ASSERT_EQ(data_added->referenced_data_handles()->size(), 1);
                    const auto handle = data_added->referenced_data_handles()->Get(0);
                    ASSERT_EQ(handle->class_id(), data_class_id);
                    ASSERT_EQ(handle->object_version(), 1);
                    ASSERT_EQ(handle->primary_key()->str(), data_primary_key);
                }
            }
        }
    }
    SUBTEST_END

    std::string keep_until_delete_data_primary_key;
    SUBTEST_BEGIN(Save And Events)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{
                CreateValueProto(context.builder, ValueUnionProto::StringValueProto,
                                 CreateStringValueProtoDirect(context.builder, "scott").Union())
        };
        const auto response = context.call_service_method(service_class_id, service_primary_key, method_saveAndEventsTest, std::move(arguments),
                                                          std::nullopt);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(), ValueUnionProto::DataReferenceValueProto);
            const auto data_ref = return_value->value_as_DataReferenceValueProto();
            ASSERT_EQ(data_ref->class_id(), data_class_id);
            keep_until_delete_data_primary_key = data_ref->primary_key()->str();
            ASSERT_EQ(keep_until_delete_data_primary_key.size(), uuid_len);
        }

        ASSERT_TRUE(response->deltas());
        ASSERT_EQ(response->deltas()->size(), 4);
        ASSERT_TRUE(response->events());
        ASSERT_EQ(response->events()->size(), 2); //two DataAdded events


        { // First Delta
            const auto delta = response->deltas()->Get(0)->bytes_nested_root();
            ASSERT_FALSE(delta->deleted());
            ASSERT_EQ(delta->class_id(), data_class_id);
            ASSERT_EQ(delta->object_version(), 1);
            ASSERT_EQ(delta->primary_key()->str(), keep_until_delete_data_primary_key);
            ASSERT_FALSE(delta->deleted_properties());
            ASSERT_TRUE(delta->properties());
            const auto properties = delta->properties();
            ASSERT_EQ(properties->size(), 1);
            const auto p = properties->Get(0);
            ASSERT_EQ(p->name()->str(), "name");
            ASSERT_EQ(p->value_bytes_nested_root()->value_as_StringValueProto()->value()->str(), "scott");
        }

        { // Second Delta
            const auto delta = response->deltas()->Get(1)->bytes_nested_root();
            ASSERT_FALSE(delta->deleted());
            ASSERT_EQ(delta->class_id(), data_class_id);
            ASSERT_EQ(delta->object_version(), 2);
            ASSERT_EQ(delta->primary_key()->str(), keep_until_delete_data_primary_key);
            ASSERT_FALSE(delta->deleted_properties());
            ASSERT_TRUE(delta->properties());
            const auto properties = delta->properties();
            ASSERT_EQ(properties->size(), 1);
            const auto p = properties->Get(0);
            ASSERT_EQ(p->name()->str(), "name");
            ASSERT_EQ(p->value_bytes_nested_root()->value_as_StringValueProto()->value()->str(), "jones");
        }

        { // Third Delta
            const auto delta = response->deltas()->Get(2)->bytes_nested_root();
            ASSERT_FALSE(delta->deleted());
            ASSERT_EQ(delta->class_id(), data_class_id);
            ASSERT_EQ(delta->object_version(), 3);
            ASSERT_EQ(delta->primary_key()->str(), keep_until_delete_data_primary_key);
            ASSERT_TRUE(delta->deleted_properties());
            ASSERT_EQ(delta->deleted_properties()->size(), 1);
            ASSERT_EQ(delta->deleted_properties()->Get(0)->str(), "name");
            ASSERT_TRUE(delta->properties());
            const auto properties = delta->properties();
            ASSERT_EQ(properties->size(), 1);
            const auto p = properties->Get(0);
            VAL_NAME(p, "associatedData");
            VAL_WOBJ(p->value_bytes_nested_root(), associated_data_class_id, associated_data_primary_key.view());
        }

        { // Fourth Delta
            const auto delta = response->deltas()->Get(3)->bytes_nested_root();
            ASSERT_FALSE(delta->deleted());
            ASSERT_EQ(delta->class_id(), associated_data_class_id);
            ASSERT_EQ(delta->object_version(), 1);
            ASSERT_EQ(delta->primary_key()->str(), associated_data_primary_key.view());
            ASSERT_FALSE(delta->deleted_properties());
            ASSERT_TRUE(delta->properties());
            const auto ps = delta->properties();
            ASSERT_EQ(ps->size(), 2);
            VAL_NAME(ps->Get(0), "firstName");
            VAL_STR(ps->Get(0)->value_bytes_nested_root(), "Scott");
            VAL_NAME(ps->Get(1), "lastName");
            VAL_STR(ps->Get(1)->value_bytes_nested_root(), "Jones");
        }

        { // First event
            const auto data_added = response->events()->Get(0)->bytes_nested_root();

            //validate the source
            ASSERT_EQ(WorkerReferenceUnionProto::ServiceReferenceValueProto, data_added->source_type());
            const auto ref = data_added->source_as_ServiceReferenceValueProto();
            ASSERT_EQ(service_class_id, ref->class_id());
            ASSERT_EQ(service_primary_key.view(), ref->primary_key()->string_view());

            ASSERT_EQ(data_added->class_id(), data_added_class_id);

            { //Validate the DataAdded event's properties
                ASSERT_EQ(data_added->properties()->size(), 1);
                const auto p = data_added->properties()->Get(0);
                ASSERT_EQ(p->name()->str(), "data");
                const auto data_ref = p->value()->value_as_DataReferenceValueProto();
                ASSERT_EQ(data_ref->class_id(), data_class_id);
                ASSERT_EQ(data_ref->primary_key()->str(), keep_until_delete_data_primary_key);
            }

            { //Validate the referenced worker object handles
                ASSERT_EQ(data_added->referenced_data_handles()->size(), 1);
                const auto handle = data_added->referenced_data_handles()->Get(0);
                ASSERT_EQ(handle->class_id(), data_class_id);
                ASSERT_EQ(handle->object_version(), 1);
                ASSERT_EQ(handle->primary_key()->str(), keep_until_delete_data_primary_key);
            }
        }

        { // Second event
            const auto data_added = response->events()->Get(1)->bytes_nested_root();

            // validate the source
            ASSERT_EQ(WorkerReferenceUnionProto::ServiceReferenceValueProto, data_added->source_type());
            const auto ref = data_added->source_as_ServiceReferenceValueProto();
            ASSERT_EQ(service_class_id, ref->class_id());
            ASSERT_EQ(service_primary_key.view(), ref->primary_key()->string_view());

            ASSERT_EQ(data_added->class_id(), data_added_class_id);

            { //Validate the DataAdded event's properties
                ASSERT_EQ(data_added->properties()->size(), 1);
                const auto p = data_added->properties()->Get(0);
                ASSERT_EQ(p->name()->str(), "data");
                const auto data_ref = p->value()->value_as_DataReferenceValueProto();
                ASSERT_EQ(data_ref->class_id(), data_class_id);
                ASSERT_EQ(data_ref->primary_key()->str(), keep_until_delete_data_primary_key);
            }

            { //Validate the referenced worker object handles
                ASSERT_EQ(data_added->referenced_data_handles()->size(), 1);
                const auto handle = data_added->referenced_data_handles()->Get(0);
                ASSERT_EQ(handle->class_id(), data_class_id);
                ASSERT_EQ(handle->object_version(), 2);
                ASSERT_EQ(handle->primary_key()->str(), keep_until_delete_data_primary_key);
            }
        }
    }
    SUBTEST_END

    SUBTEST_BEGIN(Get Object)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{
                CreateValueProto(context.builder, ValueUnionProto::StringValueProto,
                                 CreateStringValueProtoDirect(context.builder, keep_until_delete_data_primary_key.c_str()).Union())
        };
        const auto response = context.call_service_method(service_class_id, service_primary_key, method_getObjectTest, std::move(arguments),
                                                          std::nullopt);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(), ValueUnionProto::DataReferenceValueProto);
            const auto data_ref = return_value->value_as_DataReferenceValueProto();
            ASSERT_EQ(data_ref->class_id(), data_class_id);
            ASSERT_EQ(data_ref->primary_key()->str(), keep_until_delete_data_primary_key);
        }

        ASSERT_FALSE(response->deltas());
        ASSERT_FALSE(response->events());
    }
    SUBTEST_END

    SUBTEST_BEGIN(Get Data Not Found)
    {
        context.call_service_method(service_class_id, service_primary_key, method_getObjectNotFoundTest,
                                    std::nullopt, std::nullopt,
                                    estate::test::ExpectedException{
                                            "Error: [getData]: Unable to load object due to error Datastore_ObjectNotFound",
                                            "Error: [getData]: Unable to load object due to error Datastore_ObjectNotFound\n"
                                            "    at ExampleService.getDataNotFoundTest (worker://TestWorker/index.mjs:32:23)"
                                    });
    }
    SUBTEST_END

    SUBTEST_BEGIN(Revert Data To Nothing)
    {
        std::string reverted_data_primary_key;
        const auto response = context.call_service_method(service_class_id, service_primary_key, method_revertObjectToNothingTest, std::nullopt,
                                                          std::nullopt);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(), ValueUnionProto::DataReferenceValueProto);
            const auto data_ref = return_value->value_as_DataReferenceValueProto();
            ASSERT_EQ(data_ref->class_id(), data_class_id);
            reverted_data_primary_key = data_ref->primary_key()->str();
            ASSERT_EQ(reverted_data_primary_key.size(), 36);
        }

        ASSERT_FALSE(response->deltas());
        ASSERT_FALSE(response->events());
    }
    SUBTEST_END

    SUBTEST_BEGIN(Revert Object To Save)
    {
        std::string reverted_data_primary_key;
        const auto response = context.call_service_method(service_class_id, service_primary_key, method_revertObjectToSaveTest, std::nullopt,
                                                          std::nullopt);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(), ValueUnionProto::DataReferenceValueProto);
            const auto data_ref = return_value->value_as_DataReferenceValueProto();
            ASSERT_EQ(data_ref->class_id(), data_class_id);
            reverted_data_primary_key = data_ref->primary_key()->str();
            ASSERT_EQ(reverted_data_primary_key.size(), 36);
        }

        ASSERT_TRUE(response->deltas() && response->deltas()->size() == 1);
        ASSERT_FALSE(response->events());

        { // Delta
            const auto delta = response->deltas()->Get(0)->bytes_nested_root();
            ASSERT_FALSE(delta->deleted());
            ASSERT_EQ(delta->class_id(), data_class_id);
            ASSERT_EQ(delta->object_version(), 1);
            ASSERT_EQ(delta->primary_key()->str(), reverted_data_primary_key);
            ASSERT_FALSE(delta->deleted_properties());
            ASSERT_TRUE(delta->properties());
            const auto ps = delta->properties();
            ASSERT_EQ(ps->size(), 1);
            VAL_NAME(ps->Get(0), "name");
            VAL_STR(ps->Get(0)->value_bytes_nested_root(), "to save");
        }
    }
    SUBTEST_END

    SUBTEST_BEGIN(Unsaved Changes)
    {
        context.call_service_method(service_class_id, service_primary_key, method_unsavedChangesTest, std::nullopt, std::nullopt,
                                    Code::Datastore_UnsavedChanges);
    }
    SUBTEST_END

    SUBTEST_BEGIN(Revert Service)
    {
        const auto response = context.call_service_method(service_class_id, service_primary_key, method_revertSelfTest, std::nullopt, std::nullopt);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(),
                      ValueUnionProto::UndefinedValueProto); //undefined because the service's property change was reverted.
        }

        ASSERT_FALSE(response->deltas());
        ASSERT_FALSE(response->events());
    }
    SUBTEST_END

    std::string keep_until_delete_metadata_primary_key;
    SUBTEST_BEGIN(Create Service)
    {
        const auto response = context.call_service_method(service_class_id, service_primary_key, method_createServiceTest, std::nullopt,
                                                          std::nullopt);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(), ValueUnionProto::ServiceReferenceValueProto);
            const auto metadata_ref = return_value->value_as_ServiceReferenceValueProto();
            ASSERT_EQ(metadata_ref->class_id(), metadata_service_class_id);
            keep_until_delete_metadata_primary_key = metadata_ref->primary_key()->str();
            ASSERT_EQ(keep_until_delete_metadata_primary_key.size(), uuid_len_no_dashes);
        }

        ASSERT_FALSE(response->deltas());
        ASSERT_FALSE(response->events());
    }
    SUBTEST_END

    SUBTEST_BEGIN(Get Service)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{
                CreateValueProto(context.builder, ValueUnionProto::StringValueProto,
                                 CreateStringValueProtoDirect(context.builder, keep_until_delete_metadata_primary_key.c_str()).Union())
        };
        const auto response = context.call_service_method(service_class_id, service_primary_key, method_getServiceTest, std::move(arguments),
                                                          std::nullopt);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(), ValueUnionProto::ServiceReferenceValueProto);
            const auto metadata_ref = return_value->value_as_ServiceReferenceValueProto();
            ASSERT_EQ(metadata_ref->class_id(), metadata_service_class_id);
            ASSERT_EQ(metadata_ref->primary_key()->str(), keep_until_delete_metadata_primary_key);
        }

        ASSERT_FALSE(response->deltas());
        ASSERT_FALSE(response->events());
    }
    SUBTEST_END

    SUBTEST_BEGIN(Server Method Exception)
    {
        context.call_service_method(service_class_id, service_primary_key, method_invalidCallTest,
                                    std::nullopt, std::nullopt,
                                    estate::test::ExpectedException{
                                            "Error: [getService]: Incorrect number of arguments",
                                            "Error: [getService]: Incorrect number of arguments\n"
                                            "    at ExampleService.invalidCallTest (worker://TestWorker/index.mjs:91:16)"
                                    });
    }
    SUBTEST_END

    SUBTEST_BEGIN(Inbound Delta Create Object)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{};
        SET_BUILDER(context.builder);
        PrimaryKey pk{std::string{"associated"}};

        arguments.push_back(WOBJ_VAL(associated_data_class_id, pk));

        std::vector<fbs::Offset<InboundDataDeltaProto>> referenced_data_deltas{};
        {
            std::vector<fbs::Offset<NestedPropertyProto>> properties{};

            fbs::Builder inner_builder{};
            inner_builder.Finish(CreateValueProto(inner_builder, ValueUnionProto::StringValueProto,
                                                  CreateStringValueProto(inner_builder, inner_builder.CreateString("Scott")).Union()));
            properties.push_back(CreateNestedPropertyProto(context.builder, context.builder.CreateString("firstName"),
                                                           context.builder.CreateVector(inner_builder.GetBufferPointer(), inner_builder.GetSize())));
            referenced_data_deltas.push_back(
                    CreateInboundDataDeltaProto(context.builder,
                                                      associated_data_class_id,
                                                      0, //Create a new object
                                                      context.builder.CreateString(pk.view()),
                                                      context.builder.CreateVector(properties),
                                                      0));
        }

        const auto response = context.call_service_method(service_class_id, service_primary_key, method_inboundDeltaCreateObject,
                                                          std::move(arguments), referenced_data_deltas);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(), ValueUnionProto::UndefinedValueProto);
            VAL_UNDEF(return_value);
        }

        // Validate the delta
        ASSERT_TRUE(response->deltas() && response->deltas()->size() == 1);
        ASSERT_FALSE(response->events());

        { //Delta
            const auto delta = response->deltas()->Get(0)->bytes_nested_root();
            ASSERT_EQ(delta->object_version(), 1);
            ASSERT_FALSE(delta->deleted());
            ASSERT_EQ(delta->primary_key()->str(), pk.view());
            ASSERT_EQ(delta->class_id(), associated_data_class_id);
            ASSERT_FALSE(delta->deleted_properties());
            ASSERT_TRUE(delta->properties());
            ASSERT_EQ(delta->properties()->size(), 1);
            {
                const auto p = delta->properties()->Get(0);
                VAL_NAME(p, "firstName");
                VAL_STR(p->value_bytes_nested_root(), "Scott");
            }
        }
    }
    SUBTEST_END

    SUBTEST_BEGIN(Inbound Delta Add Property)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{};
        SET_BUILDER(context.builder);
        PrimaryKey pk{std::string{"associated"}};

        arguments.push_back(WOBJ_VAL(associated_data_class_id, pk));

        std::vector<fbs::Offset<InboundDataDeltaProto>> referenced_data_deltas{};
        {
            std::vector<fbs::Offset<NestedPropertyProto>> properties{};

            fbs::Builder inner_builder{};
            inner_builder.Finish(CreateValueProto(inner_builder, ValueUnionProto::StringValueProto,
                                                  CreateStringValueProto(inner_builder, inner_builder.CreateString("Jones")).Union()));
            properties.push_back(CreateNestedPropertyProto(context.builder, context.builder.CreateString("lastName"),
                                                           context.builder.CreateVector(inner_builder.GetBufferPointer(), inner_builder.GetSize())));
            referenced_data_deltas.push_back(
                    CreateInboundDataDeltaProto(context.builder,
                                                      associated_data_class_id,
                                                      1,
                                                      context.builder.CreateString(pk.view()),
                                                      context.builder.CreateVector(properties),
                                                      0));
        }

        const auto response = context.call_service_method(service_class_id, service_primary_key, method_inboundDeltaAddProperty,
                                                          std::move(arguments), referenced_data_deltas);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(), ValueUnionProto::UndefinedValueProto);
            VAL_UNDEF(return_value);
        }

        // Validate the delta
        ASSERT_TRUE(response->deltas() && response->deltas()->size() == 1);
        ASSERT_FALSE(response->events());

        { //Delta
            const auto delta = response->deltas()->Get(0)->bytes_nested_root();
            ASSERT_EQ(delta->object_version(), 2);
            ASSERT_FALSE(delta->deleted());
            ASSERT_EQ(delta->primary_key()->str(), pk.view());
            ASSERT_EQ(delta->class_id(), associated_data_class_id);
            ASSERT_FALSE(delta->deleted_properties());
            ASSERT_TRUE(delta->properties());
            ASSERT_EQ(delta->properties()->size(), 1);
            {
                const auto p = delta->properties()->Get(0);
                VAL_NAME(p, "lastName");
                VAL_STR(p->value_bytes_nested_root(), "Jones");
            }
        }
    }
    SUBTEST_END

    SUBTEST_BEGIN(Inbound Delta Delete Property)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{};
        SET_BUILDER(context.builder);
        PrimaryKey pk{std::string{"associated"}};

        arguments.push_back(WOBJ_VAL(associated_data_class_id, pk));

        std::vector<fbs::Offset<InboundDataDeltaProto>> referenced_data_deltas{};
        {
            std::vector<fbs::Offset<fbs::String>> deleted_properties{};
            deleted_properties.push_back(context.builder.CreateString("lastName"));

            referenced_data_deltas.push_back(
                    CreateInboundDataDeltaProto(context.builder,
                                                      associated_data_class_id,
                                                      2,
                                                      context.builder.CreateString(pk.view()),
                                                      0,
                                                      context.builder.CreateVector(deleted_properties)));
        }

        const auto response = context.call_service_method(service_class_id, service_primary_key, method_inboundDeltaDeleteProperty,
                                                          std::move(arguments), referenced_data_deltas);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(), ValueUnionProto::UndefinedValueProto);
            VAL_UNDEF(return_value);
        }

        // Validate the delta
        ASSERT_TRUE(response->deltas() && response->deltas()->size() == 1);
        ASSERT_FALSE(response->events());

        { //Delta
            const auto delta = response->deltas()->Get(0)->bytes_nested_root();
            ASSERT_EQ(delta->object_version(), 3);
            ASSERT_FALSE(delta->deleted());
            ASSERT_EQ(delta->primary_key()->str(), pk.view());
            ASSERT_EQ(delta->class_id(), associated_data_class_id);
            ASSERT_FALSE(delta->properties());
            ASSERT_TRUE(delta->deleted_properties() && delta->deleted_properties()->size() == 1);
            ASSERT_EQ(delta->deleted_properties()->Get(0)->str(), "lastName");
        }
    }
    SUBTEST_END

    SUBTEST_BEGIN(Inbound Delta No Changes)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{};
        SET_BUILDER(context.builder);
        PrimaryKey pk{std::string{"associated"}};

        arguments.push_back(WOBJ_VAL(associated_data_class_id, pk));

        const auto response = context.call_service_method(service_class_id, service_primary_key, method_inboundDeltaNoChanges,
                                                          std::move(arguments), std::nullopt);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(), ValueUnionProto::UndefinedValueProto);
            VAL_UNDEF(return_value);
        }

        // Validate the delta
        ASSERT_FALSE(response->deltas());
        ASSERT_FALSE(response->events());
    }
    SUBTEST_END

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Delete tests must run last

    SUBTEST_BEGIN(Delete Test);
    {
        const char *user_name = "delete-me";
        std::string user_pk;
        { // Delete Test 1
            std::vector<fbs::Offset<ValueProto>> arguments{
                    CreateValueProto(context.builder, ValueUnionProto::StringValueProto,
                                     CreateStringValueProtoDirect(context.builder, user_name).Union())
            };
            const auto response = context.call_service_method(service_class_id, service_primary_key, method_deleteTest1, std::move(arguments),
                                                              std::nullopt);
            ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

            { // Validate the response
                const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
                const auto return_value = inner_response->return_value();
                ASSERT_EQ(return_value->value_type(), ValueUnionProto::DataReferenceValueProto);
                const auto ref_value = return_value->value_as_DataReferenceValueProto();
                ASSERT_EQ(ref_value->class_id(), data_class_id);
                user_pk = ref_value->primary_key()->str();
                ASSERT_EQ(user_pk.size(), uuid_len);
            }

            ASSERT_FALSE(response->events());
            ASSERT_TRUE(response->deltas());
            ASSERT_EQ(response->deltas()->size(), 1);

            { // Delta
                const auto delta = response->deltas()->Get(0)->bytes_nested_root();
                ASSERT_FALSE(delta->deleted());
                ASSERT_EQ(delta->class_id(), data_class_id);
                ASSERT_EQ(delta->object_version(), 1);
                ASSERT_EQ(delta->primary_key()->str(), user_pk);
                ASSERT_FALSE(delta->deleted_properties());
                ASSERT_TRUE(delta->properties());
                const auto properties = delta->properties();
                ASSERT_EQ(properties->size(), 1);
                const auto p = properties->Get(0);
                ASSERT_EQ(p->name()->str(), "name");
                ASSERT_EQ(p->value_bytes_nested_root()->value_as_StringValueProto()->value()->str(), user_name);
            }
        }

        // Delete Test 2
        {
            std::vector<fbs::Offset<ValueProto>> arguments{};
            SET_BUILDER(context.builder);
            PrimaryKey pk{user_pk};
            arguments.push_back(WOBJ_VAL(data_class_id, pk));

            std::vector<fbs::Offset<InboundDataDeltaProto>> referenced_data_deltas{};
            {
                std::vector<fbs::Offset<NestedPropertyProto>> properties{};

                fbs::Builder inner_builder{};
                inner_builder.Finish(CreateValueProto(inner_builder, ValueUnionProto::StringValueProto,
                                                      CreateStringValueProto(inner_builder, inner_builder.CreateString(user_name)).Union()));
                properties.push_back(CreateNestedPropertyProto(context.builder, context.builder.CreateString("name"),
                                                               context.builder.CreateVector(inner_builder.GetBufferPointer(),
                                                                                            inner_builder.GetSize())));
                referenced_data_deltas.push_back(
                        CreateInboundDataDeltaProto(context.builder,
                                                          data_class_id,
                                                          1,
                                                          context.builder.CreateString(pk.view()),
                                                          context.builder.CreateVector(properties),
                                                          0));
            }

            const auto response = context.call_service_method(service_class_id, service_primary_key, method_deleteTest2,
                                                              std::move(arguments), referenced_data_deltas);
            ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

            { // Validate the response
                const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
                const auto return_value = inner_response->return_value();
                ASSERT_EQ(return_value->value_type(), ValueUnionProto::UndefinedValueProto);
                VAL_UNDEF(return_value);
            }

            ASSERT_FALSE(response->events());
            ASSERT_TRUE(response->deltas());
            ASSERT_EQ(response->deltas()->size(), 1);

            { // Delta
                const auto delta = response->deltas()->Get(0)->bytes_nested_root();
                ASSERT_TRUE(delta->deleted());
                ASSERT_EQ(delta->class_id(), data_class_id);
                ASSERT_EQ(delta->object_version(), 2);
                ASSERT_EQ(delta->primary_key()->str(), user_pk);
                ASSERT_FALSE(delta->deleted_properties());
                ASSERT_FALSE(delta->properties());
            }
        }

        { // Delete Test 3
            std::vector<fbs::Offset<ValueProto>> arguments{
                    CreateValueProto(context.builder, ValueUnionProto::StringValueProto,
                                     CreateStringValueProtoDirect(context.builder, user_pk.c_str()).Union())
            };
            //NOTE: If this fails you should double-check the line numbers in contract_call_service_method_tests/ServerApi/index.mjs
            //NOTE: expecting a script error
            context.call_service_method(service_class_id, service_primary_key, method_deleteTest3,
                                        std::move(arguments), std::nullopt, estate::test::ExpectedException{
                            "Error: [getData]: Unable to load object due to error Datastore_ObjectDeleted",
                            "Error: [getData]: Unable to load object due to error Datastore_ObjectDeleted\n"
                            "    at ExampleService.deleteTest3 (worker://TestWorker/index.mjs:120:16)" /* <-- Update the line numbers as needed */
                    });
        }
    }
    SUBTEST_END

    SUBTEST_BEGIN(Purge Test);
    {
        const char *user_name = "purge-me";
        std::string user_pk;
        { // Purge Test 1
            std::vector<fbs::Offset<ValueProto>> arguments{
                    CreateValueProto(context.builder, ValueUnionProto::StringValueProto,
                                     CreateStringValueProtoDirect(context.builder, user_name).Union())
            };
            const auto response = context.call_service_method(service_class_id, service_primary_key, method_purgeTest1, std::move(arguments),
                                                              std::nullopt);
            ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

            { // Validate the response
                const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
                const auto return_value = inner_response->return_value();
                ASSERT_EQ(return_value->value_type(), ValueUnionProto::DataReferenceValueProto);
                const auto ref_value = return_value->value_as_DataReferenceValueProto();
                ASSERT_EQ(ref_value->class_id(), data_class_id);
                user_pk = ref_value->primary_key()->str();
                ASSERT_EQ(user_pk.size(), uuid_len);
            }

            ASSERT_FALSE(response->events());
            ASSERT_TRUE(response->deltas());
            ASSERT_EQ(response->deltas()->size(), 1);

            { // Delta
                const auto delta = response->deltas()->Get(0)->bytes_nested_root();
                ASSERT_FALSE(delta->deleted());
                ASSERT_EQ(delta->class_id(), data_class_id);
                ASSERT_EQ(delta->object_version(), 1);
                ASSERT_EQ(delta->primary_key()->str(), user_pk);
                ASSERT_FALSE(delta->deleted_properties());
                ASSERT_TRUE(delta->properties());
                const auto properties = delta->properties();
                ASSERT_EQ(properties->size(), 1);
                const auto p = properties->Get(0);
                ASSERT_EQ(p->name()->str(), "name");
                ASSERT_EQ(p->value_bytes_nested_root()->value_as_StringValueProto()->value()->str(), user_name);
            }
        }

        // Purge Test 2
        {
            std::vector<fbs::Offset<ValueProto>> arguments{};
            SET_BUILDER(context.builder);
            PrimaryKey pk{user_pk};
            arguments.push_back(WOBJ_VAL(data_class_id, pk));

            std::vector<fbs::Offset<InboundDataDeltaProto>> referenced_data_deltas{};
            {
                std::vector<fbs::Offset<NestedPropertyProto>> properties{};

                fbs::Builder inner_builder{};
                inner_builder.Finish(CreateValueProto(inner_builder, ValueUnionProto::StringValueProto,
                                                      CreateStringValueProto(inner_builder, inner_builder.CreateString(user_name)).Union()));
                properties.push_back(CreateNestedPropertyProto(context.builder, context.builder.CreateString("name"),
                                                               context.builder.CreateVector(inner_builder.GetBufferPointer(),
                                                                                            inner_builder.GetSize())));
                referenced_data_deltas.push_back(
                        CreateInboundDataDeltaProto(context.builder,
                                                          data_class_id,
                                                          1,
                                                          context.builder.CreateString(pk.view()),
                                                          context.builder.CreateVector(properties),
                                                          0));
            }

            const auto response = context.call_service_method(service_class_id, service_primary_key, method_purgeTest2,
                                                              std::move(arguments), referenced_data_deltas);
            ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

            { // Validate the response
                const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
                const auto return_value = inner_response->return_value();
                ASSERT_EQ(return_value->value_type(), ValueUnionProto::UndefinedValueProto);
                VAL_UNDEF(return_value);
            }

            ASSERT_FALSE(response->events());
            ASSERT_TRUE(response->deltas());
            ASSERT_EQ(response->deltas()->size(), 1);

            { // Delta
                const auto delta = response->deltas()->Get(0)->bytes_nested_root();
                ASSERT_TRUE(delta->deleted());
                ASSERT_EQ(delta->class_id(), data_class_id);
                ASSERT_EQ(delta->object_version(), 2);
                ASSERT_EQ(delta->primary_key()->str(), user_pk);
                ASSERT_FALSE(delta->deleted_properties());
                ASSERT_FALSE(delta->properties());
            }
        }

        { // Delete Test 3
            std::vector<fbs::Offset<ValueProto>> arguments{
                    CreateValueProto(context.builder, ValueUnionProto::StringValueProto,
                                     CreateStringValueProtoDirect(context.builder, user_pk.c_str()).Union())
            };
            //NOTE: expecting a script error
            const auto response = context.call_service_method(service_class_id, service_primary_key, method_purgeTest3,
                                                              std::move(arguments), std::nullopt, estate::test::ExpectedException{
                            "Error: [getData]: Unable to load object due to error Datastore_ObjectNotFound",
                            "Error: [getData]: Unable to load object due to error Datastore_ObjectNotFound\n"
                            "    at ExampleService.purgeTest3 (worker://TestWorker/index.mjs:131:16)"
                    });
        }
    }
    SUBTEST_END

    SUBTEST_BEGIN(Eval Test)
    {
        //NOTE: If this fails you should double-check the line numbers in contract_call_service_method_tests/ServerApi/index.mjs
        //NOTE: expecting a script error
        context.call_service_method(service_class_id, service_primary_key, method_evalTest, std::nullopt, std::nullopt,
                                    estate::test::ExpectedException{
                                            "ReferenceError: eval is not defined",
                                            "ReferenceError: eval is not defined\n"
                                            "    at ExampleService.evalTest (worker://TestWorker/index.mjs:134:9)" /* <-- Update the line numbers as needed */
                                    });
    }
    SUBTEST_END

    SUBTEST_BEGIN(Delete Service (Self))
    {
        const auto response = context.call_service_method(service_class_id, service_primary_key, method_deleteSelfTest, std::nullopt, std::nullopt);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(), ValueUnionProto::UndefinedValueProto); //undefined because nothing is returned
        }

        ASSERT_FALSE(response->deltas());
        ASSERT_FALSE(response->events());

        context.call_service_method(service_class_id, service_primary_key, method_deleteSelfTestConfirm,
                                    std::nullopt, std::nullopt, Code::Datastore_ObjectDeleted);
    }
    SUBTEST_END
}


TEST(contract_call_service_method_tests, MethodAndSerialization) {
    SETUP(1, true, true, false);

    PrimaryKey service_primary_key{std::string{"my_service"}};
    PrimaryKey other_service_primary_key{std::string{"my_other_service"}};
    PrimaryKey metadata_primary_key{std::string{"my_metadata"}};
    PrimaryKey other_metadata_primary_key{std::string{"a different one"}};
    static const auto LAUREN_BDAY = 1048560763000; //Sun Mar 25 2003 02:52:43 GMT+0000 (Coordinated Universal Time)

    ClassId service_class_id = 1;
    ClassId other_service_class_id = 2;
    ClassId metadata_class_id = 3;
    int m = 100;
    MethodId method_testBoolean = m++;
    MethodId method_testNumber = m++;
    MethodId method_testString = m++;
    MethodId method_testObject = m++;
    MethodId method_testDataReference = m++;
    MethodId method_testServiceReference = m++;
    MethodId method_testNull = m++;
    MethodId method_testUndefined = m++;
    MethodId method_testArray = m++;
    MethodId method_testMap = m++;
    MethodId method_testSet = m++;
    MethodId method_testDate = m++;

    SUBTEST_BEGIN(Boolean)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{
                CreateValueProto(context.builder, ValueUnionProto::BooleanValueProto,
                                 CreateBooleanValueProto(context.builder, true).Union())
        };
        const auto response = context.call_service_method(service_class_id, service_primary_key, method_testBoolean, std::move(arguments),
                                                          std::nullopt);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(), ValueUnionProto::BooleanValueProto);
            ASSERT_EQ(return_value->value_as_BooleanValueProto()->value(), true);
        }
        ASSERT_FALSE(response->deltas());
        ASSERT_FALSE(response->events());
    }
    SUBTEST_END

    SUBTEST_BEGIN(Number)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{
                CreateValueProto(context.builder, ValueUnionProto::NumberValueProto,
                                 CreateNumberValueProto(context.builder, 50001).Union())
        };
        const auto response = context.call_service_method(service_class_id, service_primary_key, method_testNumber, std::move(arguments),
                                                          std::nullopt);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(), ValueUnionProto::NumberValueProto);
            ASSERT_EQ(return_value->value_as_NumberValueProto()->value(), 50001);
        }
        ASSERT_FALSE(response->deltas());
        ASSERT_FALSE(response->events());
    }
    SUBTEST_END

    SUBTEST_BEGIN(String)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{
                CreateValueProto(context.builder, ValueUnionProto::StringValueProto,
                                 CreateStringValueProto(context.builder, context.builder.CreateString("something")).Union())
        };
        const auto response = context.call_service_method(service_class_id, service_primary_key, method_testString, std::move(arguments),
                                                          std::nullopt);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(), ValueUnionProto::StringValueProto);
            ASSERT_EQ(return_value->value_as_StringValueProto()->value()->str(), "something");
        }
        ASSERT_FALSE(response->deltas());
        ASSERT_FALSE(response->events());
    }
    SUBTEST_END

    SUBTEST_BEGIN(Object)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{};
        {
            SET_BUILDER(context.builder);

            ///////////// Object properties
            std::vector<fbs::Offset<PropertyProto>> properties{};

            //scalars
            properties.push_back(PROP("booleanValue", BOOL_VAL(true)));
            properties.push_back(PROP("numberValue", NUM_VAL(6503)));
            properties.push_back(PROP("stringValue", STR_VAL("something")));
            properties.push_back(PROP("dataValue", WOBJ_VAL(metadata_class_id, metadata_primary_key)));
            properties.push_back(PROP("serviceValue", WSVC_VAL(other_service_class_id, other_service_primary_key)));
            properties.push_back(PROP("nullValue", NULL_VAL()));
            properties.push_back(PROP("undefinedValue", UNDEF_VAL()));
            properties.push_back(PROP("dateValue", DATE_VAL(LAUREN_BDAY)));

            //object
            std::vector<fbs::Offset<PropertyProto>> sub_object_properties{};
            sub_object_properties.push_back(PROP("subStringValue", STR_VAL("anything")));
            properties.push_back(PROP("objectValue", OBJ_VAL(sub_object_properties)));

            //array
            std::vector<fbs::Offset<ArrayItemValueProto>> array_items{};
            array_items.push_back(ARRAY_ITEM(0, BOOL_VAL(true)));
            array_items.push_back(ARRAY_ITEM(1, STR_VAL("whatever")));
            array_items.push_back(ARRAY_ITEM(2, WSVC_VAL(other_service_class_id, other_service_primary_key)));
            array_items.push_back(ARRAY_ITEM(3, WOBJ_VAL(metadata_class_id, metadata_primary_key)));
            properties.push_back(PROP("arrayValue", ARRAY_VAL(array_items)));

            //map
            std::vector<fbs::Offset<MapValueItemProto>> map_items{};
            // these two values will be combined with the second overwriting the first.
            map_items.push_back(MAP_ITEM(STR_VAL("key"), STR_VAL("value")));
            map_items.push_back(MAP_ITEM(STR_VAL("key"), WOBJ_VAL(metadata_class_id, metadata_primary_key)));

            // these two values will be stored as separate objects since they're not the same object
            map_items.push_back(
                    MAP_ITEM(WOBJ_VAL(metadata_class_id, metadata_primary_key), WSVC_VAL(other_service_class_id, other_service_primary_key)));
            map_items.push_back(
                    MAP_ITEM(WOBJ_VAL(metadata_class_id, metadata_primary_key), WSVC_VAL(other_service_class_id, other_service_primary_key)));

            map_items.push_back(
                    MAP_ITEM(WSVC_VAL(other_service_class_id, other_service_primary_key), WOBJ_VAL(metadata_class_id, metadata_primary_key)));

            //these two values will be combined as well with the second overwriting the first.
            map_items.push_back(MAP_ITEM(UNDEF_VAL(), NULL_VAL()));
            map_items.push_back(MAP_ITEM(UNDEF_VAL(), STR_VAL("whatever")));

            properties.push_back(PROP("mapValue", MAP_VAL(map_items)));

            //set
            std::vector<fbs::Offset<ValueProto>> set_items{};
            set_items.push_back(WOBJ_VAL(metadata_class_id, metadata_primary_key));
            set_items.push_back(WSVC_VAL(other_service_class_id, other_service_primary_key));
            set_items.push_back(WOBJ_VAL(metadata_class_id, metadata_primary_key)); //how will this work?
            set_items.push_back(STR_VAL("unique!"));
            set_items.push_back(STR_VAL("not unique")); //should come back as a single element
            set_items.push_back(STR_VAL("not unique"));
            properties.push_back(PROP("setValue", SET_VAL(set_items)));

            arguments.push_back(OBJ_VAL(properties));
        }

        std::vector<fbs::Offset<InboundDataDeltaProto>> referenced_data_deltas{};
        {
            fbs::Builder inner_builder{};
            inner_builder.Finish(CreateValueProto(inner_builder, ValueUnionProto::StringValueProto,
                                                  CreateStringValueProto(inner_builder, inner_builder.CreateString("whatever")).Union()));
            std::vector<fbs::Offset<NestedPropertyProto>> properties{};
            properties.push_back(CreateNestedPropertyProto(context.builder, context.builder.CreateString("stringProperty"),
                                                           context.builder.CreateVector(inner_builder.GetBufferPointer(), inner_builder.GetSize())));
            referenced_data_deltas.push_back(
                    CreateInboundDataDeltaProto(context.builder,
                                                      metadata_class_id,
                                                      0,
                                                      context.builder.CreateString(metadata_primary_key.view()),
                                                      context.builder.CreateVector(properties),
                                                      0));
        }
        const auto response = context.call_service_method(service_class_id, service_primary_key, method_testObject, std::move(arguments),
                                                          referenced_data_deltas);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(), ValueUnionProto::ObjectValueProto);

            const auto ps1 = return_value->value_as_ObjectValueProto()->properties();
            ASSERT_EQ(ps1->size(), 12);

            VAL_NAME(ps1->Get(0), "booleanValue");
            VAL_BOOL(ps1->Get(0)->value(), true);

            VAL_NAME(ps1->Get(1), "numberValue");
            VAL_NUM(ps1->Get(1)->value(), 6503);

            VAL_NAME(ps1->Get(2), "stringValue");
            VAL_STR(ps1->Get(2)->value(), "something");

            VAL_NAME(ps1->Get(3), "dataValue");
            VAL_WOBJ(ps1->Get(3)->value(), metadata_class_id, metadata_primary_key.view());

            VAL_NAME(ps1->Get(4), "serviceValue");
            VAL_WSVC(ps1->Get(4)->value(), other_service_class_id, other_service_primary_key.view());

            VAL_NAME(ps1->Get(5), "nullValue");
            VAL_NULL(ps1->Get(5)->value());

            VAL_NAME(ps1->Get(6), "undefinedValue");
            VAL_UNDEF(ps1->Get(6)->value());

            VAL_NAME(ps1->Get(7), "dateValue");
            VAL_DATE(ps1->Get(7)->value(), LAUREN_BDAY);

            {
                const auto op = ps1->Get(8);
                VAL_NAME(op, "objectValue");
                ASSERT_EQ(op->value()->value_type(), ValueUnionProto::ObjectValueProto);
                const auto ps2 = op->value()->value_as_ObjectValueProto()->properties();
                VAL_NAME(ps2->Get(0), "subStringValue");
                VAL_STR(ps2->Get(0)->value(), "anything");
            }

            {
                const auto ap = ps1->Get(9);
                VAL_NAME(ap, "arrayValue");
                ASSERT_EQ(ap->value()->value_type(), ValueUnionProto::ArrayValueProto);
                const auto items = ap->value()->value_as_ArrayValueProto()->items();
                ASSERT_EQ(items->size(), 4);
                VAL_BOOL(items->Get(0)->value(), true);
                VAL_STR(items->Get(1)->value(), "whatever");
                VAL_WSVC(items->Get(2)->value(), other_service_class_id, other_service_primary_key.view());
                VAL_WOBJ(items->Get(3)->value(), metadata_class_id, metadata_primary_key.view());
            }

            {
                const auto mp = ps1->Get(10);
                VAL_NAME(mp, "mapValue");
                ASSERT_EQ(mp->value()->value_type(), ValueUnionProto::MapValueProto);
                const auto items = mp->value()->value_as_MapValueProto()->items();
                ASSERT_EQ(items->size(), 5);
                VAL_STR(items->Get(0)->key(), "key");
                VAL_WOBJ(items->Get(0)->value(), metadata_class_id, metadata_primary_key.view());
                VAL_WOBJ(items->Get(1)->key(), metadata_class_id, metadata_primary_key.view());
                VAL_WSVC(items->Get(1)->value(), other_service_class_id, other_service_primary_key.view());
                VAL_WOBJ(items->Get(2)->key(), metadata_class_id, metadata_primary_key.view());
                VAL_WSVC(items->Get(2)->value(), other_service_class_id, other_service_primary_key.view());
                VAL_WSVC(items->Get(3)->key(), other_service_class_id, other_service_primary_key.view());
                VAL_WOBJ(items->Get(3)->value(), metadata_class_id, metadata_primary_key.view());
                VAL_UNDEF(items->Get(4)->key());
                VAL_STR(items->Get(4)->value(), "whatever");
            }

            {
                const auto sp = ps1->Get(11);
                VAL_NAME(sp, "setValue");
                ASSERT_EQ(sp->value()->value_type(), ValueUnionProto::SetValueProto);
                const auto items = sp->value()->value_as_SetValueProto()->items();
                ASSERT_EQ(items->size(), 5);
                VAL_WOBJ(items->Get(0), metadata_class_id, metadata_primary_key.view());
                VAL_WSVC(items->Get(1), other_service_class_id, other_service_primary_key.view());
                VAL_WOBJ(items->Get(2), metadata_class_id, metadata_primary_key.view());
                VAL_STR(items->Get(3), "unique!");
                VAL_STR(items->Get(4), "not unique");
            }
        }

        ASSERT_FALSE(response->deltas());
        ASSERT_FALSE(response->events());
    }
    SUBTEST_END

    SUBTEST_BEGIN(DataReference)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{};
        SET_BUILDER(context.builder);
        arguments.push_back(WOBJ_VAL(metadata_class_id, metadata_primary_key));

        std::vector<fbs::Offset<InboundDataDeltaProto>> referenced_data_deltas{};
        {
            std::vector<fbs::Offset<NestedPropertyProto>> properties{};

            fbs::Builder inner_builder{};
            inner_builder.Finish(CreateValueProto(inner_builder, ValueUnionProto::StringValueProto,
                                                  CreateStringValueProto(inner_builder, inner_builder.CreateString("whatever")).Union()));
            properties.push_back(CreateNestedPropertyProto(context.builder, context.builder.CreateString("stringProperty"),
                                                           context.builder.CreateVector(inner_builder.GetBufferPointer(), inner_builder.GetSize())));
            inner_builder.Reset();
            inner_builder.Finish(CreateValueProto(inner_builder, ValueUnionProto::StringValueProto,
                                                  CreateStringValueProto(inner_builder, inner_builder.CreateString("nothing at all")).Union()));
            properties.push_back(CreateNestedPropertyProto(context.builder, context.builder.CreateString("badProperty"),
                                                           context.builder.CreateVector(inner_builder.GetBufferPointer(), inner_builder.GetSize())));
            referenced_data_deltas.push_back(
                    CreateInboundDataDeltaProto(context.builder,
                                                      metadata_class_id,
                                                      0,
                                                      context.builder.CreateString(metadata_primary_key.view()),
                                                      context.builder.CreateVector(properties),
                                                      0));
        }

        const auto response = context.call_service_method(service_class_id, service_primary_key, method_testDataReference,
                                                          std::move(arguments), referenced_data_deltas);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(), ValueUnionProto::DataReferenceValueProto);
            VAL_WOBJ(return_value, metadata_class_id, metadata_primary_key.view());
        }

        // Validate the delta
        ASSERT_TRUE(response->deltas() && response->deltas()->size() == 1);
        ASSERT_FALSE(response->events());

        { //Delta
            const auto delta = response->deltas()->Get(0)->bytes_nested_root();
            ASSERT_EQ(delta->object_version(), 1);
            ASSERT_FALSE(delta->deleted());
            ASSERT_EQ(delta->primary_key()->str(), metadata_primary_key.view());
            ASSERT_EQ(delta->class_id(), metadata_class_id);
            ASSERT_FALSE(delta->deleted_properties());
            ASSERT_TRUE(delta->properties());
            ASSERT_EQ(delta->properties()->size(), 2);
            {
                const auto p = delta->properties()->Get(0);
                VAL_NAME(p, "stringProperty");
                VAL_STR(p->value_bytes_nested_root(), "whatever");
            }
            {
                const auto p = delta->properties()->Get(1);
                VAL_NAME(p, "badProperty");
                VAL_STR(p->value_bytes_nested_root(), "nothing at all");
            }
        }
    }
    SUBTEST_END

    SUBTEST_BEGIN(ServiceReference)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{};
        SET_BUILDER(context.builder);
        arguments.push_back(WSVC_VAL(other_service_class_id, other_service_primary_key));

        const auto response = context.call_service_method(service_class_id, service_primary_key, method_testServiceReference,
                                                          std::move(arguments), std::nullopt);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(), ValueUnionProto::ServiceReferenceValueProto);
            VAL_WSVC(return_value, other_service_class_id, other_service_primary_key.view());
        }

        ASSERT_FALSE(response->deltas());
        ASSERT_FALSE(response->events());
    }
    SUBTEST_END

    SUBTEST_BEGIN(Null)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{};
        SET_BUILDER(context.builder);
        arguments.push_back(NULL_VAL());

        const auto response = context.call_service_method(service_class_id, service_primary_key, method_testNull,
                                                          std::move(arguments), std::nullopt);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            VAL_NULL(return_value);
        }
        ASSERT_FALSE(response->deltas());
        ASSERT_FALSE(response->events());
    }
    SUBTEST_END

    SUBTEST_BEGIN(Undefined)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{};
        SET_BUILDER(context.builder);
        arguments.push_back(UNDEF_VAL());

        const auto response = context.call_service_method(service_class_id, service_primary_key, method_testUndefined,
                                                          std::move(arguments), std::nullopt);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            VAL_UNDEF(return_value);
        }
        ASSERT_FALSE(response->deltas());
        ASSERT_FALSE(response->events());
    }
    SUBTEST_END

    SUBTEST_BEGIN(Array)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{};
        {
            //Note: arrays are tested extensively in the Object test.
            SET_BUILDER(context.builder);
            std::vector<fbs::Offset<ArrayItemValueProto>> items{};
            items.push_back(ARRAY_ITEM(0, BOOL_VAL(true)));
            items.push_back(ARRAY_ITEM(1, STR_VAL("whatever")));
            arguments.push_back(ARRAY_VAL(items));
        }

        const auto response = context.call_service_method(service_class_id, service_primary_key, method_testArray, std::move(arguments),
                                                          std::nullopt);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(), ValueUnionProto::ArrayValueProto);
            ASSERT_TRUE(return_value->value_as_ArrayValueProto()->items());
            const auto items = return_value->value_as_ArrayValueProto()->items();
            ASSERT_EQ(items->size(), 2);
            VAL_BOOL(items->Get(0)->value(), true);
            VAL_STR(items->Get(1)->value(), "whatever");
        }

        ASSERT_FALSE(response->events());
        ASSERT_FALSE(response->deltas());
    }
    SUBTEST_END

    SUBTEST_BEGIN(Map)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{};
        {
            SET_BUILDER(context.builder);

            //note: maps are tested extensively in the Object test

            //map
            std::vector<fbs::Offset<MapValueItemProto>> items{};
            // these two values will be combined with the second overwriting the first.
            items.push_back(MAP_ITEM(STR_VAL("key"), STR_VAL("value")));
            items.push_back(MAP_ITEM(STR_VAL("key"), BOOL_VAL(true)));

            arguments.push_back(MAP_VAL(items));
        }

        const auto response = context.call_service_method(service_class_id, service_primary_key, method_testMap, std::move(arguments),
                                                          std::nullopt);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(), ValueUnionProto::MapValueProto);
            const auto items = return_value->value_as_MapValueProto()->items();
            ASSERT_TRUE(items);
            ASSERT_EQ(items->size(), 1);
            VAL_STR(items->Get(0)->key(), "key");
            VAL_BOOL(items->Get(0)->value(), true);
        }

        ASSERT_FALSE(response->deltas());
        ASSERT_FALSE(response->events());
    }
    SUBTEST_END

    SUBTEST_BEGIN(Set)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{};
        {
            SET_BUILDER(context.builder);

            //note: Sets are tested extensively in the Object test.

            //set
            std::vector<fbs::Offset<ValueProto>> set_items{};
            set_items.push_back(STR_VAL("unique!"));
            set_items.push_back(STR_VAL("not unique")); //should come back as a single element
            set_items.push_back(STR_VAL("not unique"));

            arguments.push_back(SET_VAL(set_items));
        }

        const auto response = context.call_service_method(service_class_id, service_primary_key, method_testSet, std::move(arguments),
                                                          std::nullopt);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            ASSERT_EQ(return_value->value_type(), ValueUnionProto::SetValueProto);

            const auto items = return_value->value_as_SetValueProto()->items();
            ASSERT_TRUE(items);
            ASSERT_EQ(items->size(), 2);
            VAL_STR(items->Get(0), "unique!");
            VAL_STR(items->Get(1), "not unique");
        }

        ASSERT_FALSE(response->events());
        ASSERT_FALSE(response->deltas());
    }
    SUBTEST_END

    SUBTEST_BEGIN(Date)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{};
        {
            SET_BUILDER(context.builder);
            arguments.push_back(DATE_VAL(LAUREN_BDAY));
        }

        const auto response = context.call_service_method(service_class_id, service_primary_key, method_testDate, std::move(arguments),
                                                          std::nullopt);
        ASSERT_EQ(response->response_nested_root()->value_type(), UserResponseUnionProto::CallServiceMethodResponseProto);

        { // Validate the response
            const auto inner_response = response->response_nested_root()->value_as_CallServiceMethodResponseProto();
            const auto return_value = inner_response->return_value();
            VAL_DATE(return_value, LAUREN_BDAY);
        }

        ASSERT_FALSE(response->deltas());
        ASSERT_FALSE(response->events());
    }
    SUBTEST_END
}

TEST(contract_call_service_method_tests, Duplicate) {
    SETUP(1, true, true, false);

    PrimaryKey account_service_primary_key{std::string{"my_service"}};

    ClassId account_class_id = 1;
    ClassId account_service_class_id = 2;

    int m = 100;
    MethodId method_addDuplicate = m++;

    SUBTEST_BEGIN(AddDuplicate)
    {
        const auto response = context.call_service_method(account_service_class_id, account_service_primary_key, method_addDuplicate,
                                                          std::nullopt, std::nullopt, Code::Datastore_DuplicateObject);
    }
    SUBTEST_END
}
