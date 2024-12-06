//
// Originally written by Scott R. Jones.
// Copyright (c) 2020 Warpdrive Technologies, Inc. All rights reserved.
//

#include "estate/internal/database_keys.h"

#include <string>

namespace estate {
    std::string create_object_properties_index_key(const ClassId &class_id, const PrimaryKey &primary_key) {
        std::string key_str(std::to_string(class_id));
        key_str.append(ESTATE_DB_KEY_DELIM);
        key_str.append(primary_key.view());
        key_str.append(ESTATE_DB_OBJECT_PROPERTIES_INDEX_KEY_SUFFIX);
        return std::move(key_str);
    }

    std::string create_object_instance_key(const ClassId &class_id, const PrimaryKey &primary_key) {
        std::string key_str(std::to_string(class_id));
        key_str.append(ESTATE_DB_KEY_DELIM);
        key_str.append(primary_key.view());
        key_str.append(ESTATE_DB_OBJECT_INSTANCE_KEY_SUFFIX);
        return std::move(key_str);
    }

    std::string create_property_key(const ClassId &class_id, const PrimaryKey &primary_key, const std::string_view property_name) {
        std::string key_str(std::to_string(class_id));
        key_str.append(ESTATE_DB_KEY_DELIM);
        key_str.append(primary_key.view());
        key_str.append(ESTATE_DB_KEY_DELIM);
        key_str.append(property_name);
        key_str.append(ESTATE_DB_PROPERTY_KEY_SUFFIX);
        return std::move(key_str);
    }
}
