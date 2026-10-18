#ifndef CROWNDB_TYPES_H
#define CROWNDB_TYPES_H
#include<cstdint>
#include<string>
#include<variant>
#include<vector>

enum class DataType {
    Null,
    Integer,
    Float,
    Text,
    Boolean,
    Date,
    Binary
};

class TypedValue {
public:
    using VariantType = std::variant<std::monostate, int64_t, double, std::string, bool, std::vector<uint8_t>>;

private:
    DataType type_;
    VariantType value_;

public:
    TypedValue() : type_(DataType::Null), value_(std::monostate{}) {}
    explicit TypedValue(int64_t val) : type_(DataType::Integer), value_(val) {}
    explicit TypedValue(int32_t val) : type_(DataType::Integer), value_(static_cast<int64_t>(val)) {}
    explicit TypedValue(double val) : type_(DataType::Float), value_(val) {}
    explicit TypedValue(std::string val) : type_(DataType::Text), value_(std::move(val)) {}
    explicit TypedValue(const char* val) : type_(DataType::Text), value_(std::string(val)) {}
    explicit TypedValue(bool val) : type_(DataType::Boolean), value_(val) {}
    explicit TypedValue(std::vector<uint8_t> val) : type_(DataType::Binary), value_(std::move(val)) {}
    
    static TypedValue make_date(int32_t date_encoded) {
        TypedValue tv;
        tv.type_ = DataType::Date;
        tv.value_ = static_cast<int64_t>(date_encoded);
        return tv;
    }
    
    static TypedValue make_null() {
        return TypedValue();
    }
    
    DataType type() const { return type_; }
    bool is_null() const { return type_ == DataType::Null; }
    int64_t get_integer() const { return std::get<int64_t>(value_); }
    double get_float() const { return std::get<double>(value_); }
    const std::string& get_text() const { return std::get<std::string>(value_); }
    bool get_boolean() const { return std::get<bool>(value_); }
    int32_t get_date() const { return static_cast<int32_t>(std::get<int64_t>(value_)); }
    const std::vector<uint8_t>& get_binary() const { return std::get<std::vector<uint8_t>>(value_); }
    
    std::string to_string() const {
        if (type_ == DataType::Null) {
            return "NULL";
        }
        if (type_ == DataType::Integer) {
            return std::to_string(std::get<int64_t>(value_));
        }
        if (type_ == DataType::Float) {
            return std::to_string(std::get<double>(value_));
        }
        if (type_ == DataType::Text) {
            return std::get<std::string>(value_);
        }
        if (type_ == DataType::Boolean) {
            return std::get<bool>(value_) ? "true" : "false";
        }
        if (type_ == DataType::Date) {
            const int64_t d = std::get<int64_t>(value_);
            const int year = static_cast<int>(d / 10000);
            const int month = static_cast<int>((d / 100) % 100);
            const int day = static_cast<int>(d % 100);
            std::string out = std::to_string(year) + "-";
            if (month < 10) out += "0";
            out += std::to_string(month) + "-";
            if (day < 10) out += "0";
            out += std::to_string(day);
            return out;
        }
        if (type_ == DataType::Binary) {
            return "<binary>";
        }
        return "";
    }
    
    bool operator==(const TypedValue& other) const {
        if (type_ != other.type_) return false;
        return value_ == other.value_;
    }
    
    bool operator!=(const TypedValue& other) const {
        return !(*this == other);
    }
};

#endif
