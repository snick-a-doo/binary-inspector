// Copyright © 2020-2021 Sam Varner
//
// This file is part of Inspect.
//
// Composure is free software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation, either
// version 3 of the License, or (at your option) any later version.
//
// Composure is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE.  See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with Composure.
// If not, see <http://www.gnu.org/licenses/>.

#include "../src/inspect.hh"
#include "doctest.h"

#include <fstream>
#include <iostream>

TEST_CASE("empty file")
{
    std::ifstream is("empty_file");
    auto out = inspect(is, {});
    CHECK(out.empty());
    CHECK(format_report(out).empty());
}

TEST_CASE("double")
{
    std::ifstream is("../test/test_data");
    assert(is);
    Spec spec{{"f64", {-6, 6}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 1);
    auto it = out.begin();
    CHECK(it->address == 0x0);
    CHECK(it->value == "1.230000");
    CHECK(it->type == "f64");
    auto fmt = format_report(out);
    CHECK(fmt.size() == 1);
    CHECK(fmt[0] == "0000000 0                 f64 1.230000");
}

TEST_CASE("float")
{
    std::ifstream is("../test/test_data");
    Spec spec{{"f32", {-6, 6}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 1);
    auto it = out.begin();
    CHECK(it->address == 0x4);
    CHECK(it->value == "1.903750");
    CHECK(it->type == "f32");
    auto fmt = format_report(out);
    CHECK(fmt.size() == 1);
    CHECK(fmt[0] == "0000000     4             f32 1.903750");
}

TEST_CASE("positive int")
{
    std::ifstream is("../test/test_data");
    Spec spec{{"i32", {10, 1000}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 3);
    auto it = out.begin();
    CHECK(it->address == 0x8);
    CHECK(it->value == "432");
    CHECK(it->type == "i32");
    ++it;
    CHECK(it->address == 0x47);
    CHECK(it->value == "255");
    CHECK(it->type == "i32");
    ++it;
    CHECK(it->address == 0x53);
    CHECK(it->value == "256");
    CHECK(it->type == "i32");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000000         8         i32 432");
    CHECK(fmt[1] == "0000004        7          i32 255");
    CHECK(fmt[2] == "0000005    3              i32 256");
}

TEST_CASE("positive and negative ints")
{
    std::ifstream is("../test/test_data");
    Spec spec{{"i32", {-100, 1000}}};
    auto out = inspect(is, spec);
    auto it = out.begin();
    CHECK(it->address == 0x8);
    CHECK(it->value == "432");
    CHECK(it->type == "i32");
    ++it;
    CHECK(it->address == 0x44);
    CHECK(it->value == "-1");
    CHECK(it->type == "i32");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000000         8         i32 432");
    CHECK(fmt[1] == "0000004     4             i32 -1");
}

TEST_CASE("short")
{
    std::ifstream is("../test/test_data");
    Spec spec{{"i16", {1, 0xfe}}};
    auto out = inspect(is, spec);
    auto it = out.begin();
    CHECK(it->address == 0x9);
    CHECK(it->value == "1");
    CHECK(it->type == "i16");
    ++it;
    CHECK(it->address == 0x16);
    CHECK(it->value == "111");
    CHECK(it->type == "i16");
    ++it;
    ++it;
    CHECK(it->address == 0x29);
    CHECK(it->value == "119");
    CHECK(it->type == "i16");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000000          9        i16 1");
    CHECK(fmt[1] == "0000001       6   a       i16 111");
    CHECK(fmt[2] == "0000002          9        i16 119");
}

TEST_CASE("long")
{
    std::ifstream is("../test/test_data");
    Spec spec{{"i64", {0x00ff00ff00ff0001L, 0x0100000000000000L}}};
    auto out = inspect(is, spec);
    auto it = out.begin();
    CHECK(it->address == 0xc);
    CHECK(it->value == "72038902055038719"); // 0x00ffeeffeeffeeff
    CHECK(it->type == "i64");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000000             c     i64 72038902055038719");
}

TEST_CASE("negative int")
{
    std::ifstream is("../test/test_data");
    Spec spec{{"i32", {-100, -1}}};
    auto out = inspect(is, spec);
    auto it = out.begin();
    CHECK(it->address == 0x44);
    CHECK(it->value == "-1");
    CHECK(it->type == "i32");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000004     4             i32 -1");
}

TEST_CASE("overlapping negative ints")
{
    std::ifstream is("../test/test_data");
    Spec spec{{"i32", {-1000, -1}}};
    auto out = inspect(is, spec);
    auto it = out.begin();
    CHECK(it->address == 0x43);
    CHECK(it->value == "-256");
    CHECK(it->type == "i32");
    ++it;
    CHECK(it->address == 0x44);
    CHECK(it->value == "-1");
    CHECK(it->type == "i32");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000004    3              i32 -256");
    CHECK(fmt[1] == "            4             i32 -1");
}

TEST_CASE("no match")
{
    std::ifstream is("../test/test_data");
    Spec spec{{"i32", {99, 100}}};
    auto out = inspect(is, spec);
    CHECK(out.empty());
    CHECK(format_report(out).empty());
}

TEST_CASE("all numbers")
{
    std::ifstream is("../test/test_data");
    Spec spec{{"i64", {0x00ff000000000000L, 0x0100000000000000L}},
                 {"i32", {-1000, 1000}},
                 {"i16", {1, 0xfe}},
                 {"f64", {-6, 6}},
                 {"f32", {-6, 6}}};
    auto out = inspect(is, spec);
    auto it = out.begin();
    CHECK(it->address == 0x0);
    CHECK(it->value == "1.230000");
    CHECK(it->type == "f64");
    ++it;
    CHECK(it->address == 0x4);
    CHECK(it->value == "1.903750");
    CHECK(it->type == "f32");
    ++it;
    CHECK(it->address == 0x8);
    CHECK(it->value == "432");
    CHECK(it->type == "i32");
    ++it;
    CHECK(it->address == 0x9);
    CHECK(it->value == "1");
    CHECK(it->type == "i16");
    ++it;
    CHECK(it->address == 0xc);
    CHECK(it->value == "72038902055038719"); // 0x00ffeeffeeffeeff
    CHECK(it->type == "i64");
    ++it;
    CHECK(it->address == 0x16);
    CHECK(it->value == "111");
    CHECK(it->type == "i16");
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
    std::ifstream is("../test/test_data");
    Spec spec{{"s8", {0, 3}}};
    auto out = inspect(is, spec);
    auto it = out.begin();
    CHECK(it->address == 0x0a);
    CHECK(it->value == "");
    CHECK(it->type == "s8");
    ++it;
    ++it;
    CHECK(it->address == 0x14);
    CHECK(it->value == "moo");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000000           ab      s8  ");
    CHECK(fmt[1] == "0000001     4   8         s8  moo");
}

TEST_CASE("short string")
{
    auto check = [](Report const& out) {
        auto it = out.begin();
        CHECK(it->address == 0x14);
        CHECK(it->value == "moo");
        CHECK(it->type == "s8");
        auto fmt = format_report(out);
        CHECK(fmt[0] == "0000001     4   8         s8  moo");
    };
    std::ifstream is("../test/test_data");
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
    std::ifstream is("../test/test_data");
    Spec spec{{"s8", {10, 100}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 1);
    auto it = out.begin();
    CHECK(it->address == 0x1c);
    CHECK(it->value == "w\346e\376ing w\357ll\370w"); // "wæeÞing wïlløw");
    CHECK(it->type == "s8");
    auto fmt = format_report(out);
    CHECK(fmt.size() == 1);
    CHECK(fmt[0] == "0000001             c     s8  w\346e\376ing w\357ll\370w");
}

TEST_CASE("no long ASCII string")
{
    std::ifstream is("../test/test_data");
    Spec spec{{"a8", {10, 100}}};
    auto out = inspect(is, spec);
    CHECK(out.empty());
}

TEST_CASE("repeated zeros")
{
    std::ifstream is("../test/test_data");
    Spec spec{{"i32", {-10, 10}}};
    auto out = inspect(is, spec);
    auto it = out.begin();
    CHECK(it->address == 0x44);
    CHECK(it->value == "-1");
    CHECK(it->type == "i32");
    ++it;
    CHECK(it->address == 0x48);
    CHECK(it->value == "0");
    CHECK(it->type == "i32");
    ++it;
    CHECK(it->address == 0x49);
    CHECK(it->value == "0");
    CHECK(it->type == "i32");
    std::advance(it, 7);
    CHECK(it->address == 0x50);
    CHECK(it->value == "0");
    CHECK(it->type == "i32");
    ++it;
    CHECK(it->address == 0x54);
    CHECK(it->value == "1");
    CHECK(it->type == "i32");
    auto fmt = format_report(out);
    CHECK(fmt[0] == "0000004     4             i32 -1");
    CHECK(fmt[1] == "                89abcdef  i32 0");
    CHECK(fmt[2] == "0000005 0                 i32 0");
    CHECK(fmt[3] == "            4             i32 1");
}

TEST_CASE("16-bit char string")
{
    std::ifstream is("../test/test_data_wide");
    Spec spec{{"s16", {2, 4}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 2);
    auto it = out.begin();
    CHECK(it->address == 0x12);
    CHECK(it->value == "\377moo");
    CHECK(it->type == "s16");
    ++it;
    CHECK(it->address == 0x1c);
    CHECK(it->value == "moo");
    CHECK(it->type == "s16");
    auto fmt = format_report(out);
    CHECK(fmt.size() == 2);
    CHECK(fmt[0] == "0000001   2               s16 \377moo");
    CHECK(fmt[1] == "                    c     s16 moo");
}

TEST_CASE("8-bit ASCII")
{
    std::ifstream is("../test/test_data");
    Spec spec{{"a8", {2, 4}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 2);
    auto it = out.begin();
    CHECK(it->address == 0x14);
    CHECK(it->value == "moo");
    CHECK(it->type == "a8");
    ++it;
    CHECK(it->address == 0x18);
    CHECK(it->value == "moo");
    CHECK(it->type == "a8");
    auto fmt = format_report(out);
    CHECK(fmt.size() == 1);
    CHECK(fmt[0] == "0000001     4   8         a8  moo");
}

TEST_CASE("16-bit ASCII")
{
    std::ifstream is("../test/test_data_wide");
    Spec spec{{"a16", {2, 4}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 2);
    auto it = out.begin();
    CHECK(it->address == 0x14);
    CHECK(it->value == "moo");
    CHECK(it->type == "a16");
    ++it;
    CHECK(it->address == 0x1c);
    CHECK(it->value == "moo");
    CHECK(it->type == "a16");
    auto fmt = format_report(out);
    CHECK(fmt.size() == 1);
    CHECK(fmt[0] == "0000001     4       c     a16 moo");
}

TEST_CASE("long 16-bit char string")
{
    std::ifstream is("../test/test_data_wide");
    Spec spec{{"s16", {10, 100}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 1);
    auto it = out.begin();
    CHECK(it->address == 0x24);
    CHECK(it->value == "w\346e\376ing w\357ll\370w"); // "wæeÞing wïlløw");
    CHECK(it->type == "s16");
    auto fmt = format_report(out);
    CHECK(fmt.size() == 1);
    CHECK(fmt[0] == "0000002     4             s16 w\346e\376ing w\357ll\370w");
}

TEST_CASE("no long 16-bit ASCII string")
{
    std::ifstream is("../test/test_data_wide");
    Spec spec{{"a16", {10, 100}}};
    auto out = inspect(is, spec);
    CHECK(out.empty());
}

TEST_CASE("split string")
{
    std::ifstream is("../test/test_data");
    Spec spec{{"s8", {5, 10}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 5); // false positive at 0xc: ffeeffeeffeeff00
    auto it = out.begin();
    ++it;
    CHECK(it->address == 0x2b);
    CHECK(it->value == "first");
    CHECK(it->type == "s8");
    ++it;
    CHECK(it->address == 0x31);
    CHECK(it->value == "second");
    ++it;
    CHECK(it->address == 0x38);
    CHECK(it->value == "third");
    ++it;
    CHECK(it->address == 0x3e);
    CHECK(it->value == "third");
    auto fmt = format_report(out);
    CHECK(fmt.size() == 4);
    CHECK(fmt[1] == "0000002            b      s8  first");
    CHECK(fmt[2] == "0000003  1                s8  second");
    CHECK(fmt[3] == "                8     e   s8  third");
}

TEST_CASE("split 16-bit string")
{
    std::ifstream is("../test/test_data_wide");
    Spec spec{{"s16", {5, 10}}};
    auto out = inspect(is, spec);
    CHECK(out.size() == 4);
    auto it = out.begin();
    CHECK(it->address == 0x43);
    CHECK(it->value == "first");
    CHECK(it->type == "s16");
    ++it;
    CHECK(it->address == 0x4f);
    CHECK(it->value == "second");
    ++it;
    CHECK(it->address == 0x5d);
    CHECK(it->value == "third");
    ++it;
    CHECK(it->address == 0x69);
    CHECK(it->value == "third");
    auto fmt = format_report(out);
    CHECK(fmt.size() == 4);
    CHECK(fmt[0] == "0000004    3              s16 first");
    CHECK(fmt[1] == "                       f  s16 second");
    CHECK(fmt[2] == "0000005              d    s16 third");
    CHECK(fmt[3] == "0000006          9        s16 third");
}

TEST_CASE("unknown type")
{
    std::ifstream is("../test/test_data");
    Spec spec{{"q13", {4, 10}}};
    CHECK_THROWS_AS(inspect(is, spec), unknown_type);
}

TEST_CASE("bad range")
{
    std::ifstream is("../test/test_data");
    Spec spec{{"i32", {4, -10}}};
    CHECK_THROWS_AS(inspect(is, spec), bad_range);
}
