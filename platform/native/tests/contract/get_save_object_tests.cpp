#include "../estate_test.h"

#include <gtest/gtest.h>

#include "../val_def.h"

using namespace estate;

TEST(contract_get_save_object_tests, GetAndSave) {
    SETUP(1, true, true, false);

    PrimaryKey user_pk{std::string{"scottjones"}};
    PrimaryKey metadata_pk{std::string{"scottjones"}};
    ClassId metadata_class_id = 1;
    ClassId user_class_id = 2;

    SUBTEST_BEGIN(Save New User)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{};
        SET_BUILDER(context.builder);

        arguments.push_back(WOBJ_VAL(user_class_id, user_pk));

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
                                                      user_class_id,
                                                      0, //Create a new object
                                                      context.builder.CreateString(user_pk.view()),
                                                      context.builder.CreateVector(properties),
                                                      0));
        }

        const auto response = context.save_data(referenced_data_deltas);

        ASSERT_TRUE(response->deltas() && response->deltas()->size() == 1);
        ASSERT_FALSE(response->events());

        { //Validate the handles
            auto handles = response->response_nested_root()->value_as_SaveDataResponseProto()->handles();
            ASSERT_TRUE(handles && handles->size() == 1);
            auto handle = handles->Get(0);
            ASSERT_EQ(handle->class_id(), user_class_id);
            ASSERT_EQ(handle->primary_key()->str(), user_pk.view());
            ASSERT_EQ(handle->object_version(), 1);
        }

        { //Delta
            const auto delta = response->deltas()->Get(0)->bytes_nested_root();
            ASSERT_EQ(delta->object_version(), 1);
            ASSERT_FALSE(delta->deleted());
            ASSERT_EQ(delta->primary_key()->str(), user_pk.view());
            ASSERT_EQ(delta->class_id(), user_class_id);
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

    SUBTEST_BEGIN(Get User)
    {
        const auto response = context.get_data(user_class_id, user_pk);
        const auto *data = response->response_nested_root()->value_as_GetDataResponseProto()->data();
        ASSERT_TRUE(data && data->size() == 1);
        const auto object = data->Get(0);
        ASSERT_EQ(object->object_version(), 1);
        ASSERT_EQ(object->class_id(), user_class_id);
        ASSERT_EQ(object->primary_key()->str(), user_pk.view());
        ASSERT_FALSE(object->deleted());
        ASSERT_TRUE(object->properties() && object->properties()->size() == 1);
        auto p = object->properties()->Get(0);
        VAL_NAME(p, "firstName");
        VAL_STR(p->value_bytes_nested_root(), "Scott");
    }
    SUBTEST_END

    SUBTEST_BEGIN(Attach Metadata To User)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{};
        SET_BUILDER(context.builder);

        arguments.push_back(WOBJ_VAL(user_class_id, user_pk));
        arguments.push_back(WOBJ_VAL(metadata_class_id, metadata_pk));

        std::vector<fbs::Offset<InboundDataDeltaProto>> referenced_data_deltas{};
        {
            { // Update User
                std::vector<fbs::Offset<NestedPropertyProto>> properties{};
                fbs::Builder inner_builder{};
                inner_builder.Finish(CreateValueProto(inner_builder, ValueUnionProto::StringValueProto,
                                                      CreateStringValueProto(inner_builder, inner_builder.CreateString("Jones")).Union()));
                properties.push_back(CreateNestedPropertyProto(context.builder, context.builder.CreateString("lastName"),
                                                               context.builder.CreateVector(inner_builder.GetBufferPointer(), inner_builder.GetSize())));
                inner_builder.Clear();
                inner_builder.Finish(CreateValueProto(inner_builder, ValueUnionProto::DataReferenceValueProto,
                                                      CreateDataReferenceValueProto(inner_builder, metadata_class_id, inner_builder.CreateString(metadata_pk.view())).Union()));
                properties.push_back(CreateNestedPropertyProto(context.builder, context.builder.CreateString("metadata"),
                                                               context.builder.CreateVector(inner_builder.GetBufferPointer(), inner_builder.GetSize())));
                referenced_data_deltas.push_back(
                        CreateInboundDataDeltaProto(context.builder,
                                                          user_class_id,
                                                          1,
                                                          context.builder.CreateString(user_pk.view()),
                                                          context.builder.CreateVector(properties),
                                                          0));
            }
            { // Create Metadata
                std::vector<fbs::Offset<NestedPropertyProto>> properties{};

                fbs::Builder inner_builder{};
                inner_builder.Finish(CreateValueProto(inner_builder, ValueUnionProto::StringValueProto,
                                                      CreateStringValueProto(inner_builder, inner_builder.CreateString("scott@stackless.dev")).Union()));

                properties.push_back(CreateNestedPropertyProto(context.builder, context.builder.CreateString("email"),
                                                               context.builder.CreateVector(inner_builder.GetBufferPointer(), inner_builder.GetSize())));
                referenced_data_deltas.push_back(
                        CreateInboundDataDeltaProto(context.builder,
                                                          metadata_class_id,
                                                          0, //Create a new object
                                                          context.builder.CreateString(metadata_pk.view()),
                                                          context.builder.CreateVector(properties),
                                                          0));
            }
        }

        const auto response = context.save_data(referenced_data_deltas);

        { //Validate the handles
            auto handles = response->response_nested_root()->value_as_SaveDataResponseProto()->handles();
            ASSERT_TRUE(handles && handles->size() == 2);
            {
                auto handle = handles->Get(0);
                ASSERT_EQ(handle->class_id(), user_class_id);
                ASSERT_EQ(handle->primary_key()->str(), user_pk.view());
                ASSERT_EQ(handle->object_version(), 2);
            }
            {
                auto handle = handles->Get(1);
                ASSERT_EQ(handle->class_id(), metadata_class_id);
                ASSERT_EQ(handle->primary_key()->str(), metadata_pk.view());
                ASSERT_EQ(handle->object_version(), 1);
            }
        }

        ASSERT_TRUE(response->deltas() && response->deltas()->size() == 2);
        ASSERT_FALSE(response->events());

        { //Delta for Metadata
            const auto delta = response->deltas()->Get(0)->bytes_nested_root();
            ASSERT_EQ(delta->object_version(), 1);
            ASSERT_FALSE(delta->deleted());
            ASSERT_EQ(delta->primary_key()->str(), metadata_pk.view());
            ASSERT_EQ(delta->class_id(), metadata_class_id);
            ASSERT_FALSE(delta->deleted_properties());
            ASSERT_TRUE(delta->properties());
            ASSERT_EQ(delta->properties()->size(), 1);
            {
                const auto p = delta->properties()->Get(0);
                VAL_NAME(p, "email");
                VAL_STR(p->value_bytes_nested_root(), "scott@stackless.dev");
            }
        }

        { //Delta for User
            const auto delta = response->deltas()->Get(1)->bytes_nested_root();
            ASSERT_EQ(delta->object_version(), 2);
            ASSERT_FALSE(delta->deleted());
            ASSERT_EQ(delta->primary_key()->str(), user_pk.view());
            ASSERT_EQ(delta->class_id(), user_class_id);
            ASSERT_FALSE(delta->deleted_properties());
            ASSERT_TRUE(delta->properties());
            ASSERT_EQ(delta->properties()->size(), 2);
            {
                const auto p = delta->properties()->Get(0);
                VAL_NAME(p, "lastName");
                VAL_STR(p->value_bytes_nested_root(), "Jones");
            }
            {
                const auto p = delta->properties()->Get(1);
                VAL_NAME(p, "metadata");
                VAL_WOBJ(p->value_bytes_nested_root(), metadata_class_id, metadata_pk.view());
            }
        }
    }
    SUBTEST_END

    SUBTEST_BEGIN(Get User And Metadata By Getting User)
    {
        const auto response = context.get_data(user_class_id, user_pk);
        const auto *data = response->response_nested_root()->value_as_GetDataResponseProto()->data();
        ASSERT_TRUE(data && data->size() == 2);
        { //User object
            const auto object = data->Get(0);
            ASSERT_EQ(object->object_version(), 2);
            ASSERT_EQ(object->class_id(), user_class_id);
            ASSERT_EQ(object->primary_key()->str(), user_pk.view());
            ASSERT_FALSE(object->deleted());
            ASSERT_TRUE(object->properties() && object->properties()->size() == 3);
            {
                auto p = object->properties()->Get(0);
                VAL_NAME(p, "firstName");
                VAL_STR(p->value_bytes_nested_root(), "Scott");
            }
            {
                auto p = object->properties()->Get(1);
                VAL_NAME(p, "lastName");
                VAL_STR(p->value_bytes_nested_root(), "Jones");
            }
            {
                auto p = object->properties()->Get(2);
                VAL_NAME(p, "metadata");
                VAL_WOBJ(p->value_bytes_nested_root(), metadata_class_id, metadata_pk.view());
            }
        }

        { //Metadata object
            const auto object = data->Get(1);
            ASSERT_EQ(object->object_version(), 1);
            ASSERT_EQ(object->class_id(), metadata_class_id);
            ASSERT_EQ(object->primary_key()->str(), metadata_pk.view());
            ASSERT_FALSE(object->deleted());
            ASSERT_TRUE(object->properties() && object->properties()->size() == 1);
            {
                auto p = object->properties()->Get(0);
                VAL_NAME(p, "email");
                VAL_STR(p->value_bytes_nested_root(), "scott@stackless.dev");
            }
        }
    }
    SUBTEST_END

    SUBTEST_BEGIN(Get Not Exist Class ID)
    {
        context.get_data(55, user_pk, Code::Datastore_ObjectNotFound);
    }
    SUBTEST_END

    SUBTEST_BEGIN(Get Not Exist Class PK)
    {
        context.get_data(user_class_id, PrimaryKey{std::string{"bogus"}}, Code::Datastore_ObjectNotFound);
    }
    SUBTEST_END

    SUBTEST_BEGIN(Save Reject Invalid Version)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{};
        SET_BUILDER(context.builder);

        arguments.push_back(WOBJ_VAL(user_class_id, user_pk));

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
                                                      user_class_id,
                                                      55,
                                                      context.builder.CreateString(user_pk.view()),
                                                      context.builder.CreateVector(properties),
                                                      0));
        }

        context.save_data(referenced_data_deltas, Code::Datastore_ClientHasVersionInvalid);
    }
    SUBTEST_END

    SUBTEST_BEGIN(Save Reject Duplicate)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{};
        SET_BUILDER(context.builder);

        arguments.push_back(WOBJ_VAL(user_class_id, user_pk));

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
                                                      user_class_id,
                                                      0,
                                                      context.builder.CreateString(user_pk.view()),
                                                      context.builder.CreateVector(properties),
                                                      0));
        }

        context.save_data(referenced_data_deltas, Code::Datastore_DuplicateObject);
    }
    SUBTEST_END

    SUBTEST_BEGIN(Save Reject Old)
    {
        std::vector<fbs::Offset<ValueProto>> arguments{};
        SET_BUILDER(context.builder);

        arguments.push_back(WOBJ_VAL(user_class_id, user_pk));

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
                                                      user_class_id,
                                                      1,
                                                      context.builder.CreateString(user_pk.view()),
                                                      context.builder.CreateVector(properties),
                                                      0));
        }

        context.save_data(referenced_data_deltas, Code::Datastore_MustGetLatestObject);
    }
    SUBTEST_END
}
