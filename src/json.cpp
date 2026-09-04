#include "hodgeledger/json.hpp"

#include <cmath>
#include <cstdio>
#include <iomanip>
#include <stdexcept>

namespace hodgeledger {

std::string json_escape(std::string_view s) {
    std::string o;
    o.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
        case '"':
            o += "\\\"";
            break;
        case '\\':
            o += "\\\\";
            break;
        case '\n':
            o += "\\n";
            break;
        case '\r':
            o += "\\r";
            break;
        case '\t':
            o += "\\t";
            break;
        default:
            if (c < 0x20) {
                char buf[8];
                std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                o += buf;
            } else {
                o.push_back(static_cast<char>(c));
            }
        }
    }
    return o;
}

JsonWriter::JsonWriter() {
    out_ << std::setprecision(17);
}

void JsonWriter::indent() {
    for (int i = 0; i < depth_; ++i) {
        out_ << "  ";
    }
}

void JsonWriter::push(bool obj) {
    begin_value();
    out_ << (obj ? '{' : '[');
    ++depth_;
    counts_.push_back(0);
    is_obj_.push_back(obj);
}

void JsonWriter::begin_value() {
    if (counts_.empty()) {
        return;
    }
    if (after_key_) {
        after_key_ = false;
        return;
    }
    if (counts_.back() > 0) {
        out_ << ',';
    }
    out_ << '\n';
    indent();
    ++counts_.back();
}

void JsonWriter::begin_object() { push(true); }

void JsonWriter::begin_array() { push(false); }

void JsonWriter::pop(char close) {
    if (counts_.empty()) {
        throw std::logic_error("json pop mismatch");
    }
    const int n = counts_.back();
    counts_.pop_back();
    is_obj_.pop_back();
    --depth_;
    if (n > 0) {
        out_ << '\n';
        indent();
    }
    out_ << close;
    if (!counts_.empty()) {
        // parent already counted this container as a value
    }
}

void JsonWriter::end_object() { pop('}'); }

void JsonWriter::end_array() { pop(']'); }

void JsonWriter::key(std::string_view k) {
    if (counts_.empty() || !is_obj_.back()) {
        throw std::logic_error("json key outside object");
    }
    if (counts_.back() > 0) {
        out_ << ',';
    }
    out_ << '\n';
    indent();
    out_ << '"' << json_escape(k) << "\": ";
    ++counts_.back();
    after_key_ = true;
}

void JsonWriter::value(std::nullptr_t) {
    begin_value();
    out_ << "null";
}

void JsonWriter::value(bool v) {
    begin_value();
    out_ << (v ? "true" : "false");
}

void JsonWriter::value(int v) { value(static_cast<long long>(v)); }

void JsonWriter::value(long long v) {
    begin_value();
    out_ << v;
}

void JsonWriter::value(double v) {
    begin_value();
    if (!std::isfinite(v)) {
        out_ << "null";
        return;
    }
    if (std::abs(v) < 1e-15) {
        out_ << 0;
        return;
    }
    out_ << v;
}

void JsonWriter::value(std::string_view v) {
    begin_value();
    out_ << '"' << json_escape(v) << '"';
}

void JsonWriter::value(const char* v) { value(std::string_view(v ? v : "")); }

void JsonWriter::value(const std::string& v) { value(std::string_view(v)); }

void JsonWriter::value_raw(std::string_view json) {
    begin_value();
    out_ << json;
}

} // namespace hodgeledger
