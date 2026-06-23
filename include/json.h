#pragma once

// Self-contained minimal JSON parser/serializer.
// Supports: null, bool, integer, double, string, array, object.
// No external dependencies.

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <cstdint>
#include <cctype>

namespace pjson {

class Json {
public:
    enum class Type { Null, Bool, Int, Double, String, Array, Object };

    Json() : type_(Type::Null) {}
    Json(std::nullptr_t) : type_(Type::Null) {}
    Json(bool v) : type_(Type::Bool), bool_(v) {}
    Json(int v) : type_(Type::Int), int_(v) {}
    Json(long long v) : type_(Type::Int), int_(v) {}
    Json(double v) : type_(Type::Double), double_(v) {}
    Json(const char* v) : type_(Type::String), string_(v) {}
    Json(const std::string& v) : type_(Type::String), string_(v) {}

    static Json MakeObject() { Json j; j.type_ = Type::Object; return j; }
    static Json MakeArray() { Json j; j.type_ = Type::Array; return j; }

    Type type() const { return type_; }
    bool is_null() const { return type_ == Type::Null; }
    bool is_object() const { return type_ == Type::Object; }
    bool is_array() const { return type_ == Type::Array; }
    bool is_string() const { return type_ == Type::String; }
    bool is_number() const { return type_ == Type::Int || type_ == Type::Double; }
    bool is_bool() const { return type_ == Type::Bool; }

    // Object access (creates entry as Object if needed)
    Json& operator[](const std::string& key) {
        if (type_ != Type::Object) { type_ = Type::Object; }
        return object_[key];
    }

    // Array push
    void push_back(const Json& value) {
        if (type_ != Type::Array) { type_ = Type::Array; }
        array_.push_back(value);
    }

    bool contains(const std::string& key) const {
        return type_ == Type::Object && object_.find(key) != object_.end();
    }

    const std::vector<Json>& items() const { return array_; }
    const std::map<std::string, Json>& fields() const { return object_; }

    // Typed getters with defaults
    std::string as_string(const std::string& def = "") const {
        if (type_ == Type::String) return string_;
        return def;
    }
    int as_int(int def = 0) const {
        if (type_ == Type::Int) return static_cast<int>(int_);
        if (type_ == Type::Double) return static_cast<int>(double_);
        return def;
    }
    bool as_bool(bool def = false) const {
        if (type_ == Type::Bool) return bool_;
        return def;
    }

    // Navigate object path safely
    std::string get_string(const std::string& key, const std::string& def = "") const {
        if (type_ == Type::Object) {
            auto it = object_.find(key);
            if (it != object_.end()) return it->second.as_string(def);
        }
        return def;
    }
    int get_int(const std::string& key, int def = 0) const {
        if (type_ == Type::Object) {
            auto it = object_.find(key);
            if (it != object_.end()) return it->second.as_int(def);
        }
        return def;
    }
    bool get_bool(const std::string& key, bool def = false) const {
        if (type_ == Type::Object) {
            auto it = object_.find(key);
            if (it != object_.end()) return it->second.as_bool(def);
        }
        return def;
    }
    // Returns child object/value (Null if missing)
    Json get_child(const std::string& key) const {
        if (type_ == Type::Object) {
            auto it = object_.find(key);
            if (it != object_.end()) return it->second;
        }
        return Json();
    }

    // Serialize
    std::string dump(int indent = -1) const {
        std::ostringstream os;
        dump_internal(os, indent, 0);
        return os.str();
    }

    // Parse. Throws std::runtime_error on failure.
    static Json parse(const std::string& text) {
        size_t pos = 0;
        skip_ws(text, pos);
        Json result = parse_value(text, pos);
        skip_ws(text, pos);
        return result;
    }

private:
    Type type_;
    bool bool_ = false;
    long long int_ = 0;
    double double_ = 0.0;
    std::string string_;
    std::vector<Json> array_;
    std::map<std::string, Json> object_;

    static void escape_string(std::ostringstream& os, const std::string& s) {
        os << '"';
        for (char c : s) {
            switch (c) {
            case '"': os << "\\\""; break;
            case '\\': os << "\\\\"; break;
            case '\b': os << "\\b"; break;
            case '\f': os << "\\f"; break;
            case '\n': os << "\\n"; break;
            case '\r': os << "\\r"; break;
            case '\t': os << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    os << buf;
                } else {
                    os << c;
                }
            }
        }
        os << '"';
    }

    void dump_internal(std::ostringstream& os, int indent, int depth) const {
        const bool pretty = indent >= 0;
        std::string pad = pretty ? std::string(static_cast<size_t>(indent) * (depth + 1), ' ') : "";
        std::string pad_close = pretty ? std::string(static_cast<size_t>(indent) * depth, ' ') : "";
        switch (type_) {
        case Type::Null: os << "null"; break;
        case Type::Bool: os << (bool_ ? "true" : "false"); break;
        case Type::Int: os << int_; break;
        case Type::Double: os << double_; break;
        case Type::String: escape_string(os, string_); break;
        case Type::Array: {
            if (array_.empty()) { os << "[]"; break; }
            os << '[';
            for (size_t i = 0; i < array_.size(); ++i) {
                if (pretty) os << '\n' << pad;
                array_[i].dump_internal(os, indent, depth + 1);
                if (i + 1 < array_.size()) os << ',';
            }
            if (pretty) os << '\n' << pad_close;
            os << ']';
            break;
        }
        case Type::Object: {
            if (object_.empty()) { os << "{}"; break; }
            os << '{';
            size_t i = 0;
            for (const auto& kv : object_) {
                if (pretty) os << '\n' << pad;
                escape_string(os, kv.first);
                os << ':';
                if (pretty) os << ' ';
                kv.second.dump_internal(os, indent, depth + 1);
                if (++i < object_.size()) os << ',';
            }
            if (pretty) os << '\n' << pad_close;
            os << '}';
            break;
        }
        }
    }

    static void skip_ws(const std::string& s, size_t& pos) {
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\n' || s[pos] == '\r')) {
            ++pos;
        }
    }

    static Json parse_value(const std::string& s, size_t& pos) {
        skip_ws(s, pos);
        if (pos >= s.size()) throw std::runtime_error("Unexpected end of JSON");
        char c = s[pos];
        switch (c) {
        case '{': return parse_object(s, pos);
        case '[': return parse_array(s, pos);
        case '"': return Json(parse_string(s, pos));
        case 't': case 'f': return parse_bool(s, pos);
        case 'n': parse_null(s, pos); return Json();
        default: return parse_number(s, pos);
        }
    }

    static Json parse_object(const std::string& s, size_t& pos) {
        Json obj = MakeObject();
        ++pos; // skip {
        skip_ws(s, pos);
        if (pos < s.size() && s[pos] == '}') { ++pos; return obj; }
        while (true) {
            skip_ws(s, pos);
            if (pos >= s.size() || s[pos] != '"') throw std::runtime_error("Expected string key in object");
            std::string key = parse_string(s, pos);
            skip_ws(s, pos);
            if (pos >= s.size() || s[pos] != ':') throw std::runtime_error("Expected ':' in object");
            ++pos;
            Json value = parse_value(s, pos);
            obj.object_[key] = value;
            skip_ws(s, pos);
            if (pos >= s.size()) throw std::runtime_error("Unterminated object");
            if (s[pos] == ',') { ++pos; continue; }
            if (s[pos] == '}') { ++pos; break; }
            throw std::runtime_error("Expected ',' or '}' in object");
        }
        return obj;
    }

    static Json parse_array(const std::string& s, size_t& pos) {
        Json arr = MakeArray();
        ++pos; // skip [
        skip_ws(s, pos);
        if (pos < s.size() && s[pos] == ']') { ++pos; return arr; }
        while (true) {
            Json value = parse_value(s, pos);
            arr.array_.push_back(value);
            skip_ws(s, pos);
            if (pos >= s.size()) throw std::runtime_error("Unterminated array");
            if (s[pos] == ',') { ++pos; continue; }
            if (s[pos] == ']') { ++pos; break; }
            throw std::runtime_error("Expected ',' or ']' in array");
        }
        return arr;
    }

    static std::string parse_string(const std::string& s, size_t& pos) {
        std::string result;
        ++pos; // skip opening quote
        while (pos < s.size()) {
            char c = s[pos++];
            if (c == '"') return result;
            if (c == '\\') {
                if (pos >= s.size()) break;
                char esc = s[pos++];
                switch (esc) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'u': {
                    if (pos + 4 > s.size()) throw std::runtime_error("Invalid \\u escape");
                    unsigned int code = 0;
                    for (int i = 0; i < 4; ++i) {
                        char h = s[pos++];
                        code <<= 4;
                        if (h >= '0' && h <= '9') code |= (h - '0');
                        else if (h >= 'a' && h <= 'f') code |= (h - 'a' + 10);
                        else if (h >= 'A' && h <= 'F') code |= (h - 'A' + 10);
                        else throw std::runtime_error("Invalid hex in \\u escape");
                    }
                    // Encode as UTF-8
                    if (code < 0x80) {
                        result += static_cast<char>(code);
                    } else if (code < 0x800) {
                        result += static_cast<char>(0xC0 | (code >> 6));
                        result += static_cast<char>(0x80 | (code & 0x3F));
                    } else {
                        result += static_cast<char>(0xE0 | (code >> 12));
                        result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                        result += static_cast<char>(0x80 | (code & 0x3F));
                    }
                    break;
                }
                default: throw std::runtime_error("Invalid escape character");
                }
            } else {
                result += c;
            }
        }
        throw std::runtime_error("Unterminated string");
    }

    static Json parse_bool(const std::string& s, size_t& pos) {
        if (s.compare(pos, 4, "true") == 0) { pos += 4; return Json(true); }
        if (s.compare(pos, 5, "false") == 0) { pos += 5; return Json(false); }
        throw std::runtime_error("Invalid boolean literal");
    }

    static void parse_null(const std::string& s, size_t& pos) {
        if (s.compare(pos, 4, "null") == 0) { pos += 4; return; }
        throw std::runtime_error("Invalid null literal");
    }

    static Json parse_number(const std::string& s, size_t& pos) {
        size_t start = pos;
        bool is_double = false;
        if (pos < s.size() && (s[pos] == '-' || s[pos] == '+')) ++pos;
        while (pos < s.size()) {
            char c = s[pos];
            if (c >= '0' && c <= '9') { ++pos; }
            else if (c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-') { is_double = true; ++pos; }
            else break;
        }
        std::string num = s.substr(start, pos - start);
        if (num.empty()) throw std::runtime_error("Invalid number");
        try {
            if (is_double) return Json(std::stod(num));
            return Json(static_cast<long long>(std::stoll(num)));
        } catch (...) {
            throw std::runtime_error("Invalid number: " + num);
        }
    }
};

} // namespace pjson
