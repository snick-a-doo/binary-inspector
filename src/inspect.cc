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

#include "inspect.hh"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <istream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

/// Find the next number of type T within the given range.
/// @return An Entry for the number. If an appropriate number wasn't found, return a
///    default Entry. Caller can check if the stream is good or if Entry::address is -1.
template <typename T>
Entry read_number(std::istream& is, T low, T high, T min)
{
    while(is)
    {
        auto start = is.tellg();
        T value;
        is.read(reinterpret_cast<char*>(&value), sizeof value);
        // Advance 1 byte
        is.seekg(start + std::istream::pos_type(1));
        if (low <= value && value <= high && (std::abs(value) >= min || value == 0))
        {
            std::ostringstream is;
            is << value;
            return {start, is.str(), ""};
        }
    }
    return {};
};

template <typename T>
Entry read_string(std::istream& is, size_t low, size_t high)
{
    if (!is)
        return {};
    // Find the bounds of the next null-terminated ASCII string that's within the length
    // limits.
    std::string out;
    out.reserve(high);
    auto start = is.tellg();

    // Start looking for a new string.
    auto restart = [&is, &out, &start]() {
        out.clear();
        // Advance 1 byte from where the last character was read.
        start = is.tellg() - std::istream::pos_type(sizeof(T) - 1);
        is.seekg(start);
    };

    while (is)
    {
        T c;
        is.read(reinterpret_cast<char*>(&c), sizeof c);
        auto c_low = static_cast<unsigned char>(c);
        auto const len = out.length();
        // Check for end of a long enough string.
        if (c_low == '\0' || c_low == '\t' || c_low == '\n' || c_low == '\r')
        {
            if (len >= low)
                return {start, out, ""};
            restart();
        }
        // Add a good character to the string. isprint() depends on the locale set in
        // inspect(). If the locale is C it says whether the character is ASCII. If the
        // locale is iso88591 it says whether it's in the Latin-1 set.
        else if (std::isprint(c_low) && c_low == c && len < high)
            out.push_back(c_low);
        else
        {
            // Bad character or too long.
            // Consume any remaining characters from an over-long string.
            while (is && std::isprint(c_low) && c_low == c)
            {
                is.read(reinterpret_cast<char*>(&c), sizeof c);
                c_low = static_cast<unsigned char>(c);
            }
            // Start looking for the next string at the next byte. This requires rewinding
            // the stream if T is wider than 1 byte.
            restart();
        }
    }
    return {};
}

template <typename T, typename R>
Entry read_next(std::istream& is, R low, R high, R min)
{
    return read_number<T>(is, low, high, min);
}

template <>
Entry read_next<char8_t, size_t>(std::istream& is, size_t low, size_t high, size_t)
{
    return read_string<char8_t>(is, low, high);
}

template <>
Entry read_next<char16_t, size_t>(std::istream& is, size_t low, size_t high, size_t)
{
    return read_string<char16_t>(is, low, high);
}

/// Get everything in the stream that matches the given filter.
template <typename T, typename R = T>
void find(std::istream& is, Filter const& filter, Report& out)
{
    R low, high, min;
    std::istringstream is_low(filter.range.low);
    std::istringstream is_high(filter.range.high);
    std::istringstream is_min(filter.range.min);
    // setbase(0) gives prefix-dependent parsing: 0 for octal, 0x for hex.
    is_low >> std::setbase(0) >> low;
    is_high >> std::setbase(0) >> high;
    is_min >> std::setbase(0) >> min;
    if (low > high)
        throw(bad_range{filter.range});
    while (true)
    {
        auto entry = read_next<T>(is, low, high, min);
        if (!is)
            return;
        out.emplace_back(entry.address, entry.value, filter.type);
    }
}

/// Sort entries by stream position.
bool constexpr operator<(Entry const& a, Entry const& b) noexcept
{
    auto a_addr = a.address >> 4;
    auto b_addr = b.address >> 4;
    // Sort by 16-byte "row". Sort by name of type within a row.  Note that this may put
    // some entries out of address order, but allows a more orderly presentation with
    // groping of repeated values.
    return a_addr < b_addr || (a_addr == b_addr && a.type < b.type);
}

Report inspect(std::istream& is_in, Spec const& spec)
{
    std::string content((std::istreambuf_iterator<char>(is_in)), std::istreambuf_iterator<char>());
    std::istringstream is(content);
    Report out;
    auto const start = is.tellg();
    for (auto const& filter : spec)
    {
        is.clear(); // Recover from EOF in previous iteration.
        is.seekg(start);
        if (filter.type == "f64")
            find<double>(is, filter, out);
        else if (filter.type == "f32")
            find<float>(is, filter, out);
        else if (filter.type == "i64")
            find<int64_t>(is, filter, out);
        else if (filter.type == "i32")
            find<int32_t>(is, filter, out);
        else if (filter.type == "i16")
            find<int16_t>(is, filter, out);
        else if (filter.type == "s8")
        {
            std::setlocale(LC_ALL, "en_US.iso88591"); // isprint() -> Latin-1
            find<char8_t, size_t>(is, filter, out);
        }
        else if (filter.type == "s16")
        {
            std::setlocale(LC_ALL, "en_US.iso88591"); // isprint() -> Latin-1
            find<char16_t, size_t>(is, filter, out);
        }
        else if (filter.type == "a8")
        {
            std::setlocale(LC_ALL, "C"); // isprint() -> ASCII
            find<char8_t, size_t>(is, filter, out);
        }
        else if (filter.type == "a16")
        {
            std::setlocale(LC_ALL, "C"); // isprint() -> ASCII
            find<char16_t, size_t>(is, filter, out);
        }
        else
            throw(unknown_type(filter.type));
    }
    std::stable_sort(out.begin(), out.end());
    return out;
}

std::vector<std::string> format_report(Report const& report)
{
    int constexpr addr_width = 8;
    std::vector<std::string> out;
    Entry last_entry;
    for (auto const& entry : report)
    {
        auto const& [addr, value, type] = entry;
        std::ostringstream pos;
        pos << std::setfill('0') << std::setw(addr_width) << std::hex << addr;
        auto lsd = pos.str()[7];
        // If a value occurs multiple times on the same 16-byte line, show just one entry
        // in the report but mark the LSD of each address.
        bool same_line = addr >> 4 == last_entry.address >> 4;
        if (same_line && type == last_entry.type && value == last_entry.value)
        {
            out.back()[addr_width + (addr & 0xf)] = lsd;
            continue;
        }
        // Format the line.
        std::ostringstream line;
        // Don't repeat the address if it's the same up to the LSD.
        line << (last_entry.address != -1 && same_line
                 ? std::string(addr_width - 1, ' ')
                 : pos.str().substr(0, addr_width - 1))
             << ' ';
        // Show the LSD in its column.
        std::string byte(0x12, ' '); // Include 2 characters of padding.
        byte[addr & 0xf] = lsd;
        line << byte
             << std::setw(4) << std::left << type
             << value;
        out.push_back(line.str());
        last_entry = entry;
    }
    return out;
}
