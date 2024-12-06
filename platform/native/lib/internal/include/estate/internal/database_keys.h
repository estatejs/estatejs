//
// Originally written by Scott R. Jones.
// Copyright (c) 2020 Warpdrive Technologies, Inc. All rights reserved.
//

#pragma once

#include <estate/runtime/model_types.h>

#define ESTATE_DB_KEY_DELIM "|"
#define ESTATE_DB_OBJECT_INSTANCE_KEY_SUFFIX "|I"
#define ESTATE_DB_OBJECT_PROPERTIES_INDEX_KEY_SUFFIX "|PI"
#define ESTATE_DB_PROPERTY_KEY_SUFFIX "|P"
#define ESTATE_DB_WORKER_INDEX_KEY "worker_index"
#define ESTATE_DB_ENGINE_SOURCE_KEY "engine_data"
#define ESTATE_DB_WORKER_VERSION_KEY "worker_version"

namespace estate {
    std::string create_object_instance_key(const ClassId &class_id, const PrimaryKey &primary_key);
    std::string create_object_properties_index_key(const ClassId &class_id, const PrimaryKey &primary_key);
    std::string create_property_key(const ClassId &class_id, const PrimaryKey &primary_key, const std::string_view property_name);
}
