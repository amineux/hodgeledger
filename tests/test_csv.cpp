#include "hodgeledger/csv.hpp"
#include "hodgeledger/json.hpp"

#include <gtest/gtest.h>

using namespace hodgeledger;

TEST(Csv, QuotedComma) {
    auto f = parse_csv_line("a,\"b,c\",d");
    ASSERT_EQ(f.size(), 3u);
    EXPECT_EQ(f[0], "a");
    EXPECT_EQ(f[1], "b,c");
    EXPECT_EQ(f[2], "d");
}

TEST(Csv, DoubledQuotes) {
    auto f = parse_csv_line("\"say \"\"hi\"\"\",x");
    ASSERT_EQ(f.size(), 2u);
    EXPECT_EQ(f[0], "say \"hi\"");
    EXPECT_EQ(f[1], "x");
}

TEST(Json, RoundTripShape) {
    JsonWriter w;
    w.begin_object();
    w.key("n");
    w.value(3);
    w.key("ok");
    w.value(true);
    w.key("xs");
    w.begin_array();
    w.value(1.5);
    w.value("z");
    w.end_array();
    w.end_object();
    const std::string s = w.str();
    EXPECT_NE(s.find("\"n\": 3"), std::string::npos);
    EXPECT_NE(s.find("\"ok\": true"), std::string::npos);
    EXPECT_NE(s.find("1.5"), std::string::npos);
}
