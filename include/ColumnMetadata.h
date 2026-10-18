#ifndef CROWNDB_COLUMN_METADATA_H
#define CROWNDB_COLUMN_METADATA_H

#include "Types.h"
#include <string>

class ColumnMetadata {
private:
    std::string name_;
    DataType type_;
    int32_t size_;
    bool nullable_;

public:
    ColumnMetadata() : name_(""), type_(DataType::Null), size_(0), nullable_(true) {}
    ColumnMetadata(std::string name, DataType type, int32_t size = 0, bool nullable = true) : name_(std::move(name)), type_(type), size_(size), nullable_(nullable) {}

    const std::string& name() const { return name_; }
    DataType type() const { return type_; }
    int32_t size() const { return size_; }
    bool nullable() const { return nullable_; }

    bool operator==(const ColumnMetadata& other) const {
        return name_ == other.name_ && type_ == other.type_ && size_ == other.size_ && nullable_ == other.nullable_;
    }

    bool operator!=(const ColumnMetadata& other) const {
        return !(*this == other);
    }
};

#endif
