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

#include <getopt.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

/// Write the bytes of the value to the stream.
template <typename T> void write(std::ostream& os, T value)
{
    os.write(reinterpret_cast<char*>(&value), sizeof value);
}

/// Write a char string to the stream.
template <> void write(std::ostream& os, char const* str)
{
    os << str << std::ends;
}

/// Write a wide string to a stream.
template <> void write(std::ostream& os, char16_t const* str)
{
    do
        os.write(reinterpret_cast<char const*>(str), sizeof(char16_t));
    while (*str++);
}

int main(int argc, char** argv)
{
    bool wide = false;
    char c;
    while ((c = getopt(argc, argv, "u")) != -1)
    {
        if (c == 'u')
            wide = true;
    }
    if (argc - optind != 1)
    {
        std::cerr << "Usage: write [-u] FILE\n";
        exit(-1);
    }
    std::string file = argv[optind];

    std::ofstream os(file);
    write(os, 1.23); // 3ff3 ae14 7ae1 47ae -> ae47 e17a 14ae f33f
    write(os, 432); // 1b0 -> b001 0000
    write(os, 0x00ffeeffeeffeeffL); // ffee... = 72038902055038719
    if (wide)
    {
        write(os, u"moo"); // moo -> 6d6f 6f, no preceeding zero
        write(os, u"moo"); // moo -> 6d6f 6f
        write(os, u"w\346e\376ing w\357ll\370w"); // "wæeÞing wïlløw");
        // Offset the next string by 1 byte.
        char offset = 0;
        os << offset;
        write(os, u"first\tsecond\nthird"); // 3 strings
        write(os, u"third");
    }
    else
    {
        write(os, "moo"); // moo -> 6d00 6f00 6f00, no preceeding zero
        write(os, "moo"); // moo -> 6d00 6f00 6f00
        write(os, "w\346e\376ing w\357ll\370w"); // "wæeÞing wïlløw");
        write(os, "first\tsecond\nthird"); // 3 strings
        write(os, "third");
    }
    write(os, -1); // ffff ffff
    write(os, 0);
    write(os, 0);
    write(os, 0);
    write(os, 1); // 0100 0000
    return os.good();
}
