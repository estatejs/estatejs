//
// Originally written by Scott R. Jones.
// Copyright (c) 2021 Warpdrive Technologies, Inc. All rights reserved.
//

#pragma once

#define SET_BUILDER(b) auto& __builder = b;
#define PROP(n, v) CreatePropertyProto(__builder, __builder.CreateString(n), v)
#define ARRAY_ITEM(i, v) CreateArrayItemValueProto(__builder, i, v)
#define MAP_ITEM(k, v) CreateMapValueItemProto(__builder, k, v)
#define STR_VAL(s) CreateValueProto(__builder, ValueUnionProto::StringValueProto, CreateStringValueProto(__builder, __builder.CreateString(s)).Union())
#define BOOL_VAL(v) CreateValueProto(__builder, ValueUnionProto::BooleanValueProto, CreateBooleanValueProto(__builder, v).Union())
#define NUM_VAL(v) CreateValueProto(__builder, ValueUnionProto::NumberValueProto, CreateNumberValueProto(__builder, v).Union())
#define OBJ_VAL(pvec) CreateValueProto(__builder, ValueUnionProto::ObjectValueProto, CreateObjectValueProto(__builder, __builder.CreateVector(pvec)).Union())
#define WOBJ_VAL(cid, pk) CreateValueProto(__builder, ValueUnionProto::DataReferenceValueProto, CreateDataReferenceValueProto(__builder, cid, __builder.CreateString(pk.view())).Union())
#define WSVC_VAL(cid, pk) CreateValueProto(__builder, ValueUnionProto::ServiceReferenceValueProto, CreateServiceReferenceValueProto(__builder, cid, __builder.CreateString(pk.view())).Union())
#define NULL_VAL() CreateValueProto(__builder, ValueUnionProto::NullValueProto,CreateNullValueProto(__builder).Union())
#define UNDEF_VAL() CreateValueProto(__builder, ValueUnionProto::UndefinedValueProto,CreateUndefinedValueProto(__builder).Union())
#define ARRAY_VAL(aivec) CreateValueProto(__builder, ValueUnionProto::ArrayValueProto, CreateArrayValueProto(__builder, __builder.CreateVector(aivec)).Union())
#define MAP_VAL(mivec) CreateValueProto(__builder, ValueUnionProto::MapValueProto, CreateMapValueProto(__builder, __builder.CreateVector(mivec)).Union())
#define SET_VAL(sivec) CreateValueProto(__builder, ValueUnionProto::SetValueProto, CreateSetValueProto(__builder, __builder.CreateVector(sivec)).Union())
#define DATE_VAL(v) CreateValueProto(__builder, ValueUnionProto::DateValueProto, CreateDateValueProto(__builder, v).Union())

#define VAL_NAME(v, n) \
ASSERT_EQ(v->name()->str(), n)
#define VAL_BOOL(v, c) \
ASSERT_EQ(v->value_type(), ValueUnionProto::BooleanValueProto); \
ASSERT_EQ(v->value_as_BooleanValueProto()->value(), c)
#define VAL_NUM(v, c) \
ASSERT_EQ(v->value_type(), ValueUnionProto::NumberValueProto); \
ASSERT_EQ(v->value_as_NumberValueProto()->value(), c)
#define VAL_STR(v, c) \
ASSERT_EQ(v->value_type(), ValueUnionProto::StringValueProto); \
ASSERT_EQ(v->value_as_StringValueProto()->value()->str(), c)
#define VAL_WOBJ(v, cid, pk_str) \
ASSERT_EQ(v->value_type(), ValueUnionProto::DataReferenceValueProto); \
ASSERT_EQ(v->value_as_DataReferenceValueProto()->class_id(), cid); \
ASSERT_EQ(v->value_as_DataReferenceValueProto()->primary_key()->str(), pk_str);
#define VAL_WSVC(v, cid, pk_str) \
ASSERT_EQ(v->value_type(), ValueUnionProto::ServiceReferenceValueProto); \
ASSERT_EQ(v->value_as_ServiceReferenceValueProto()->class_id(), cid); \
ASSERT_EQ(v->value_as_ServiceReferenceValueProto()->primary_key()->str(), pk_str);
#define VAL_NULL(v) \
ASSERT_EQ(v->value_type(), ValueUnionProto::NullValueProto)
#define VAL_UNDEF(v) \
ASSERT_EQ(v->value_type(), ValueUnionProto::UndefinedValueProto)
#define VAL_DATE(v, c) \
ASSERT_EQ(v->value_type(), ValueUnionProto::DateValueProto);\
ASSERT_EQ(v->value_as_DateValueProto()->value(), c);
