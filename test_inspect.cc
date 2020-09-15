#include "inspect.hh"
#include "doctest.h"

#include <fstream>
#include <iostream>

TEST_CASE("empty file")
{
    std::ifstream is("../empty_file");
    auto out = inspect(is, {});
    CHECK(out.empty());
    CHECK(format_report(out).empty());
}

TEST_CASE("double")
{
    std::ifstream is("../test_data");
    Spec spec = {{"f64", {-6, 6}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 1);
    CHECK(out[0].address == 0x0);
    CHECK(out[0].value == "1.230000");
    CHECK(out[0].type == "f64");
    auto fmt = format_report(out);
    CHECK(fmt.size() == 1);
    CHECK(fmt[0] == "0000000 0                 f64 1.230000");
}

TEST_CASE("float")
{
    std::ifstream is("../test_data");
    Spec spec = {{"f32", {-6, 6}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 1);
    CHECK(out[0].address == 0x4);
    CHECK(out[0].value == "1.903750");
    CHECK(out[0].type == "f32");
    auto fmt = format_report(out);
    CHECK(fmt.size() == 1);
    CHECK(fmt[0] == "0000000     4             f32 1.903750");
}

TEST_CASE("positive int")
{
    std::ifstream is("../test_data");
    Spec spec = {{"i32", {10, 1000}}};
    auto out = inspect(is, spec);
    CHECK(out[0].address == 0x8);
    CHECK(out[0].value == "432");
    CHECK(out[0].type == "i32");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000000         8         i32 432");
}

TEST_CASE("short")
{
    std::ifstream is("../test_data");
    Spec spec = {{"i16", {1, 0xfe}}};
    auto out = inspect(is, spec);
    CHECK(out[0].address == 0x9);
    CHECK(out[0].value == "1");
    CHECK(out[0].type == "i16");
    CHECK(out[1].address == 0x16);
    CHECK(out[1].value == "111");
    CHECK(out[1].type == "i16");
    CHECK(out[2].address == 0x25);
    CHECK(out[2].value == "119");
    CHECK(out[2].type == "i16");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000000          9        i16 1");
    CHECK(fmt[1] == "0000001       6           i16 111");
    CHECK(fmt[2] == "0000002      5            i16 119");
}

TEST_CASE("long")
{
    std::ifstream is("../test_data");
    Spec spec = {{"i64", {0x00ff00ff00ff0001L, 0x0100000000000000L}}};
    auto out = inspect(is, spec);
    CHECK(out[0].address == 0xc);
    CHECK(out[0].value == "71777214294589695"); // 0x00ff00ff00ff00ff
    CHECK(out[0].type == "i64");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000000             c     i64 71777214294589695");
}

TEST_CASE("negative int")
{
    std::ifstream is("../test_data");
    Spec spec = {{"i32", {-100, 0}}};
    auto out = inspect(is, spec);
    CHECK(out[0].address == 0x27);
    CHECK(out[0].value == "-1");
    CHECK(out[0].type == "i32");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000002        7          i32 -1");
}

TEST_CASE("overlapping negative ints")
{
    std::ifstream is("../test_data");
    Spec spec = {{"i32", {-1000, 0}}};
    auto out = inspect(is, spec);
    CHECK(out[0].address == 0x26);
    CHECK(out[0].value == "-256");
    CHECK(out[0].type == "i32");
    CHECK(out[1].address == 0x27);
    CHECK(out[1].value == "-1");
    CHECK(out[1].type == "i32");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000002       6           i32 -256");
    CHECK(fmt[1] == "               7          i32 -1");
}

TEST_CASE("no match")
{
    std::ifstream is("../test_data");
    Spec spec = {{"i32", {99, 100}}};
    auto out = inspect(is, spec);
    CHECK(out.empty());
    CHECK(format_report(out).empty());
}

TEST_CASE("two things")
{
    std::ifstream is("../test_data");
    Spec spec = {{"i32", {-100, 1000}}};
    auto out = inspect(is, spec);
    CHECK(out[0].address == 0x8);
    CHECK(out[0].value == "432");
    CHECK(out[0].type == "i32");
    CHECK(out[1].address == 0x27);
    CHECK(out[1].value == "-1");
    CHECK(out[0].type == "i32");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000000         8         i32 432");
    CHECK(fmt[1] == "0000002        7          i32 -1");
}

TEST_CASE("all numbers")
{
    std::ifstream is("../test_data");
    Spec spec = {{"i64", {0x00ff00ff00ff0001L, 0x0100000000000000L}},
                 {"i32", {-1000, 1000}},
                 {"i16", {1, 0xfe}},
                 {"f64", {-6, 6}},
                 {"f32", {-6, 6}}};
    auto out = inspect(is, spec);
    CHECK(out[0].address == 0x0);
    CHECK(out[0].value == "1.230000");
    CHECK(out[0].type == "f64");
    CHECK(out[1].address == 0x4);
    CHECK(out[1].value == "1.903750");
    CHECK(out[1].type == "f32");
    CHECK(out[2].address == 0x8);
    CHECK(out[2].value == "432");
    CHECK(out[2].type == "i32");
    CHECK(out[3].address == 0x9);
    CHECK(out[3].value == "1");
    CHECK(out[3].type == "i16");
    CHECK(out[4].address == 0xc);
    CHECK(out[4].value == "71777214294589695"); // 0x00ff00ff00ff00ff
    CHECK(out[4].type == "i64");
    CHECK(out[5].address == 0x16);
    CHECK(out[5].value == "111");
    CHECK(out[5].type == "i16");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000000 0                 f64 1.230000");
    CHECK(fmt[1] == "            4             f32 1.903750");
    CHECK(fmt[2] == "                8         i32 432");
    CHECK(fmt[3] == "                 9        i16 1");
    CHECK(fmt[4] == "                    c     i64 71777214294589695");
    CHECK(fmt[5] == "0000001       6           i16 111");
}

TEST_CASE("short string")
{
    std::ifstream is("../test_data");
    Spec spec = {{"s8", {0, 3}}};
    auto out = inspect(is, spec);
    CHECK(out[0].address == 0x14);
    CHECK(out[0].value == "moo");
    CHECK(out[0].type == "s8");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000001     4             s8  moo");
}

TEST_CASE("long string")
{
    std::ifstream is("../test_data");
    Spec spec = {{"s8", {10, 100}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 1);
    CHECK(out[0].address == 0x18);
    CHECK(out[0].value == "weeping willow");
    CHECK(out[0].type == "s8");
    auto fmt = format_report(out);
    CHECK(fmt.size() == 1);
    CHECK(fmt[0] == "0000001         8         s8  weeping willow");
}

TEST_CASE("repeated zeros")
{
    std::ifstream is("../test_data");
    Spec spec = {{"i32", {-10, 10}}};
    auto out = inspect(is, spec);
    CHECK(out[0].address == 0x27);
    CHECK(out[0].value == "-1");
    CHECK(out[0].type == "i32");
    CHECK(out[1].address == 0x2b);
    CHECK(out[1].value == "0");
    CHECK(out[1].type == "i32");
    CHECK(out[2].address == 0x2c);
    CHECK(out[2].value == "0");
    CHECK(out[2].type == "i32");
    CHECK(out[9].address == 0x33);
    CHECK(out[9].value == "0");
    CHECK(out[9].type == "i32");
    CHECK(out[10].address == 0x37);
    CHECK(out[10].value == "1");
    CHECK(out[10].type == "i32");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000002        7          i32 -1");
    CHECK(fmt[1] == "                   bcdef  i32 0");
    CHECK(fmt[2] == "0000003 0123              i32 0");
    CHECK(fmt[3] == "               7          i32 1");
}
