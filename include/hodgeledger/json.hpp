#pragma once

#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace hodgeledger {

class JsonWriter {
public:
    JsonWriter();

    void begin_object();
    void end_object();
    void begin_array();
    void end_array();
    void key(std::string_view k);

    void value(std::nullptr_t);
    void value(bool v);
    void value(int v);
    void value(long long v);
    void value(double v);
    void value(std::string_view v);
    void value(const char* v);
    void value(const std::string& v);
    void value_raw(std::string_view json);

    [[nodiscard]] std::string str() const { return out_.str(); }

private:
    void indent();
    void push(bool obj);
    void pop(char close);
    void begin_value();

    std::ostringstream out_;
    std::vector<int> counts_;
    std::vector<char> is_obj_;
    int depth_ = 0;
    bool after_key_ = false;
};

std::string json_escape(std::string_view s);

} // namespace hodgeledger
