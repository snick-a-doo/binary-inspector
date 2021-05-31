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
    Spec spec{{"f64", {-6, 6}}};
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
    Spec spec{{"f32", {-6, 6}}};
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
    Spec spec{{"i32", {10, 1000}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 3);
    CHECK(out[0].address == 0x8);
    CHECK(out[0].value == "432");
    CHECK(out[0].type == "i32");
    CHECK(out[1].address == 0x47);
    CHECK(out[1].value == "255");
    CHECK(out[1].type == "i32");
    CHECK(out[2].address == 0x53);
    CHECK(out[2].value == "256");
    CHECK(out[2].type == "i32");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000000         8         i32 432");
    CHECK(fmt[1] == "0000004        7          i32 255");
    CHECK(fmt[2] == "0000005    3              i32 256");
}

TEST_CASE("positive and negative ints")
{
    std::ifstream is("../test_data");
    Spec spec{{"i32", {-100, 1000}}};
    auto out = inspect(is, spec);
    CHECK(out[0].address == 0x8);
    CHECK(out[0].value == "432");
    CHECK(out[0].type == "i32");
    CHECK(out[1].address == 0x44);
    CHECK(out[1].value == "-1");
    CHECK(out[1].type == "i32");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000000         8         i32 432");
    CHECK(fmt[1] == "0000004     4             i32 -1");
}

TEST_CASE("short")
{
    std::ifstream is("../test_data");
    Spec spec{{"i16", {1, 0xfe}}};
    auto out = inspect(is, spec);
    CHECK(out[0].address == 0x9);
    CHECK(out[0].value == "1");
    CHECK(out[0].type == "i16");
    CHECK(out[1].address == 0x16);
    CHECK(out[1].value == "111");
    CHECK(out[1].type == "i16");
    CHECK(out[3].address == 0x29);
    CHECK(out[3].value == "119");
    CHECK(out[3].type == "i16");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000000          9        i16 1");
    CHECK(fmt[1] == "0000001       6   a       i16 111");
    CHECK(fmt[2] == "0000002          9        i16 119");
}

TEST_CASE("long")
{
    std::ifstream is("../test_data");
    Spec spec{{"i64", {0x00ff00ff00ff0001L, 0x0100000000000000L}}};
    auto out = inspect(is, spec);
    CHECK(out[0].address == 0xc);
    CHECK(out[0].value == "72038902055038719"); // 0x00ffeeffeeffeeff
    CHECK(out[0].type == "i64");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000000             c     i64 72038902055038719");
}

TEST_CASE("negative int")
{
    std::ifstream is("../test_data");
    Spec spec{{"i32", {-100, -1}}};
    auto out = inspect(is, spec);
    CHECK(out[0].address == 0x44);
    CHECK(out[0].value == "-1");
    CHECK(out[0].type == "i32");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000004     4             i32 -1");
}

TEST_CASE("overlapping negative ints")
{
    std::ifstream is("../test_data");
    Spec spec{{"i32", {-1000, -1}}};
    auto out = inspect(is, spec);
    CHECK(out[0].address == 0x43);
    CHECK(out[0].value == "-256");
    CHECK(out[0].type == "i32");
    CHECK(out[1].address == 0x44);
    CHECK(out[1].value == "-1");
    CHECK(out[1].type == "i32");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000004    3              i32 -256");
    CHECK(fmt[1] == "            4             i32 -1");
}

TEST_CASE("no match")
{
    std::ifstream is("../test_data");
    Spec spec{{"i32", {99, 100}}};
    auto out = inspect(is, spec);
    CHECK(out.empty());
    CHECK(format_report(out).empty());
}

TEST_CASE("all numbers")
{
    std::ifstream is("../test_data");
    Spec spec{{"i64", {0x00ff000000000000L, 0x0100000000000000L}},
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
    CHECK(out[4].value == "72038902055038719"); // 0x00ffeeffeeffeeff
    CHECK(out[4].type == "i64");
    CHECK(out[5].address == 0x16);
    CHECK(out[5].value == "111");
    CHECK(out[5].type == "i16");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000000 0                 f64 1.230000");
    CHECK(fmt[1] == "            4             f32 1.903750");
    CHECK(fmt[2] == "                8         i32 432");
    CHECK(fmt[3] == "                 9        i16 1");
    CHECK(fmt[4] == "                    c     i64 72038902055038719");
    CHECK(fmt[5] == "0000001       6   a       i16 111");
}

TEST_CASE("zero-length string")
{
    std::ifstream is("../test_data");
    Spec spec{{"s8", {0, 3}}};
    auto out = inspect(is, spec);
    CHECK(out[0].address == 0x0a);
    CHECK(out[0].value == "");
    CHECK(out[0].type == "s8");
    CHECK(out[2].address == 0x14);
    CHECK(out[2].value == "moo");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000000           ab      s8  ");
    CHECK(fmt[1] == "0000001     4   8         s8  moo");
}

TEST_CASE("short string")
{
    auto check = [](const Report& out) {
        CHECK(out[0].address == 0x14);
        CHECK(out[0].value == "moo");
        CHECK(out[0].type == "s8");
        auto fmt = format_report(out);
        CHECK(fmt[0] == "0000001     4   8         s8  moo");
    };
    std::ifstream is("../test_data");
    SUBCASE("within")
    {
        Spec spec{{"s8", {2, 4}}};
        check(inspect(is, spec));
    }
    SUBCASE("at min")
    {
        Spec spec{{"s8", {3, 5}}};
        check(inspect(is, spec));
    }
    SUBCASE("at max")
    {
        Spec spec{{"s8", {2, 3}}};
        check(inspect(is, spec));
    }
    SUBCASE("at min and max")
    {
        Spec spec{{"s8", {3, 3}}};
        check(inspect(is, spec));
    }
}

TEST_CASE("long string")
{
    std::ifstream is("../test_data");
    Spec spec{{"s8", {10, 100}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 1);
    CHECK(out[0].address == 0x1c);
    CHECK(out[0].value == "w\346e\376ing w\357ll\370w"); // "wæeÞing wïlløw");
    CHECK(out[0].type == "s8");
    auto fmt = format_report(out);
    CHECK(fmt.size() == 1);
    CHECK(fmt[0] == "0000001             c     s8  w\346e\376ing w\357ll\370w");
}

TEST_CASE("no long ASCII string")
{
    std::ifstream is("../test_data");
    Spec spec{{"a8", {10, 100}}};
    auto out = inspect(is, spec);
    CHECK(out.empty());
}

TEST_CASE("repeated zeros")
{
    std::ifstream is("../test_data");
    Spec spec{{"i32", {-10, 10}}};
    auto out = inspect(is, spec);
    CHECK(out[0].address == 0x44);
    CHECK(out[0].value == "-1");
    CHECK(out[0].type == "i32");
    CHECK(out[1].address == 0x48);
    CHECK(out[1].value == "0");
    CHECK(out[1].type == "i32");
    CHECK(out[2].address == 0x49);
    CHECK(out[2].value == "0");
    CHECK(out[2].type == "i32");
    CHECK(out[9].address == 0x50);
    CHECK(out[9].value == "0");
    CHECK(out[9].type == "i32");
    CHECK(out[10].address == 0x54);
    CHECK(out[10].value == "1");
    CHECK(out[10].type == "i32");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000004     4             i32 -1");
    CHECK(fmt[1] == "                89abcdef  i32 0");
    CHECK(fmt[2] == "0000005 0                 i32 0");
    CHECK(fmt[3] == "            4             i32 1");
}

TEST_CASE("16-bit char string")
{
    std::ifstream is("../test_data_wide");
    Spec spec{{"s16", {2, 4}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 2);
    CHECK(out[0].address == 0x12);
    CHECK(out[0].value == "\377moo");
    CHECK(out[0].type == "s16");
    CHECK(out[1].address == 0x1c);
    CHECK(out[1].value == "moo");
    CHECK(out[1].type == "s16");
    auto fmt = format_report(out);
    CHECK(fmt.size() == 2);
    CHECK(fmt[0] == "0000001   2               s16 \377moo");
    CHECK(fmt[1] == "                    c     s16 moo");
}

TEST_CASE("8-bit ASCII")
{
    std::ifstream is("../test_data");
    Spec spec{{"a8", {2, 4}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 2);
    CHECK(out[0].address == 0x14);
    CHECK(out[0].value == "moo");
    CHECK(out[0].type == "a8");
    CHECK(out[1].address == 0x18);
    CHECK(out[1].value == "moo");
    CHECK(out[1].type == "a8");
    auto fmt = format_report(out);
    CHECK(fmt.size() == 1);
    CHECK(fmt[0] == "0000001     4   8         a8  moo");
}

TEST_CASE("16-bit ASCII")
{
    std::ifstream is("../test_data_wide");
    Spec spec{{"a16", {2, 4}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 2);
    CHECK(out[0].address == 0x14);
    CHECK(out[0].value == "moo");
    CHECK(out[0].type == "a16");
    CHECK(out[1].address == 0x1c);
    CHECK(out[1].value == "moo");
    CHECK(out[1].type == "a16");
    auto fmt = format_report(out);
    CHECK(fmt.size() == 1);
    CHECK(fmt[0] == "0000001     4       c     a16 moo");
}

TEST_CASE("long 16-bit char string")
{
    std::ifstream is("../test_data_wide");
    Spec spec{{"s16", {10, 100}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 1);
    CHECK(out[0].address == 0x24);
    CHECK(out[0].value == "w\346e\376ing w\357ll\370w"); // "wæeÞing wïlløw");
    CHECK(out[0].type == "s16");
    auto fmt = format_report(out);
    CHECK(fmt.size() == 1);
    CHECK(fmt[0] == "0000002     4             s16 w\346e\376ing w\357ll\370w");
}

TEST_CASE("no long 16-bit ASCII string")
{
    std::ifstream is("../test_data_wide");
    Spec spec{{"a16", {10, 100}}};
    auto out = inspect(is, spec);
    CHECK(out.empty());
}

TEST_CASE("split string")
{
    std::ifstream is("../test_data");
    Spec spec{{"s8", {5, 10}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 5); // false positive at 0xc: ffeeffeeffeeff00
    CHECK(out[1].address == 0x2b);
    CHECK(out[1].value == "first");
    CHECK(out[1].type == "s8");
    CHECK(out[2].address == 0x31);
    CHECK(out[2].value == "second");
    CHECK(out[3].address == 0x38);
    CHECK(out[3].value == "third");
    CHECK(out[4].address == 0x3e);
    CHECK(out[4].value == "third");
    auto fmt = format_report(out);
    CHECK(fmt.size() == 4);
    CHECK(fmt[1] == "0000002            b      s8  first");
    CHECK(fmt[2] == "0000003  1                s8  second");
    CHECK(fmt[3] == "                8     e   s8  third");
}

TEST_CASE("split 16-bit string")
{
    std::ifstream is("../test_data_wide");
    Spec spec{{"s16", {5, 10}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 4);
    CHECK(out[0].address == 0x43);
    CHECK(out[0].value == "first");
    CHECK(out[0].type == "s16");
    CHECK(out[1].address == 0x4f);
    CHECK(out[1].value == "second");
    CHECK(out[2].address == 0x5d);
    CHECK(out[2].value == "third");
    CHECK(out[3].address == 0x69);
    CHECK(out[3].value == "third");
    auto fmt = format_report(out);
    CHECK(fmt.size() == 4);
    CHECK(fmt[0] == "0000004    3              s16 first");
    CHECK(fmt[1] == "                       f  s16 second");
    CHECK(fmt[2] == "0000005              d    s16 third");
    CHECK(fmt[3] == "0000006          9        s16 third");
}

TEST_CASE("unknown type")
{
    std::ifstream is("../test_data");
    Spec spec{{"q13", {4, 10}}};
    CHECK_THROWS_AS(inspect(is, spec), unknown_type);
}

TEST_CASE("bad range")
{
    std::ifstream is("../test_data");
    Spec spec{{"i32", {4, -10}}};
    CHECK_THROWS_AS(inspect(is, spec), bad_range);
}
