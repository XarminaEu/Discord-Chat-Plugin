// Minimal nlohmann/json stub for compilation without vcpkg
// For full functionality, install nlohmann/json via vcpkg

#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <string>
#include <memory>
#include <sstream>

namespace nlohmann {

class json {
public:
    enum value_t {
        null,
        object,
        array,
        string,
        boolean,
        number_integer,
        number_float
    };

    json() : type_(null) {}
    json(const std::string& value) : type_(string), string_value_(value) {}
    json(const char* value) : type_(string), string_value_(value) {}
    json(int value) : type_(number_integer), int_value_(value) {}
    json(double value) : type_(number_float), float_value_(value) {}
    json(bool value) : type_(boolean), bool_value_(value) {}

    static json object() {
        json j;
        j.type_ = value_t::object;
        return j;
    }

    static json array() {
        json j;
        j.type_ = value_t::array;
        return j;
    }

    json& operator[](const std::string& key) {
        if (type_ != object) type_ = object;
        return *this;
    }

    json& operator[](size_t index) {
        if (type_ != array) type_ = array;
        return *this;
    }

    std::string dump(int indent = -1) const {
        std::stringstream ss;
        switch (type_) {
        case string:
            ss << "\"" << string_value_ << "\"";
            break;
        case number_integer:
            ss << int_value_;
            break;
        case number_float:
            ss << float_value_;
            break;
        case boolean:
            ss << (bool_value_ ? "true" : "false");
            break;
        case object:
            ss << "{}";
            break;
        case array:
            ss << "[]";
            break;
        case null:
            ss << "null";
            break;
        }
        return ss.str();
    }

    template<typename T>
    T value(const std::string& key, const T& default_value) const {
        return default_value;
    }

    template<typename T>
    T get() const {
        return T();
    }

private:
    value_t type_;
    std::string string_value_;
    int int_value_ = 0;
    double float_value_ = 0.0;
    bool bool_value_ = false;
};

}
