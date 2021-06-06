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
Entry read_number(std::istream& is, T low, T high, bool absolute)
{
    while(is)
    {
        auto start = is.tellg();
        T value;
        is.read(reinterpret_cast<char*>(&value), sizeof value);
        T v = absolute ? std::abs(value) : value;
        // Advance 1 byte
        is.seekg(start + std::istream::pos_type(1));
        if (low <= v && v <= high)
            return {start, std::to_string(value), ""};
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
            while (std::isprint(c_low) && c_low == c)
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
Entry read_next(std::istream& is, R low, R high)
{
    return read_number<T>(is, low, high, false);
}

template <>
Entry read_next<char8_t, size_t>(std::istream& is, size_t low, size_t high)
{
    return read_string<char8_t>(is, low, high);
}

template <>
Entry read_next<char16_t, size_t>(std::istream& is, size_t low, size_t high)
{
    return read_string<char16_t>(is, low, high);
}

/// Get everything in the stream that matches the given filter.
template <typename T, typename R = T>
void find(std::istream& is, std::string const& type, R low, R high, Report& out)
{
    while (true)
    {
        auto entry = read_next<T>(is, low, high);
        if (!is)
            return;
        out.insert({entry.address, entry.value, type});
    }
}

/// Sort entries by stream position.
bool operator<(Entry const& a, Entry const& b)
{
    return a.address < b.address;
}

Report inspect(std::istream& is_in, Spec const& spec)
{
    std::string content((std::istreambuf_iterator<char>(is_in)), std::istreambuf_iterator<char>());
    std::istringstream is(content);
    Report out;
    auto const start = is.tellg();
    for (auto const& filter : spec)
    {
        if (filter.range.low > filter.range.high)
            throw(bad_range{filter.range.low, filter.range.high});
        is.clear(); // Recover from EOF in previous iteration.
        is.seekg(start);
        if (filter.type == "f64")
            find<double>(is, filter.type,
                         std::pow(10, filter.range.low),
                         std::pow(10, filter.range.high),
                         out);
        else if (filter.type == "f32")
            find<float>(is, filter.type,
                        std::pow(10, filter.range.low),
                        std::pow(10, filter.range.high),
                        out);
        else if (filter.type == "i64")
            find<int64_t>(is, filter.type, filter.range.low, filter.range.high, out);
        else if (filter.type == "i32")
            find<int32_t>(is, filter.type, filter.range.low, filter.range.high, out);
        else if (filter.type == "i16")
            find<int16_t>(is, filter.type, filter.range.low, filter.range.high, out);
        else if (filter.type == "s8")
        {
            std::setlocale(LC_ALL, "en_US.iso88591"); // isprint() -> Latin-1
            find<char8_t, size_t>(is, filter.type, filter.range.low, filter.range.high, out);
        }
        else if (filter.type == "s16")
        {
            std::setlocale(LC_ALL, "en_US.iso88591"); // isprint() -> Latin-1
            find<char16_t, size_t>(is, filter.type, filter.range.low, filter.range.high, out);
        }
        else if (filter.type == "a8")
        {
            std::setlocale(LC_ALL, "C"); // isprint() -> ASCII
            find<char8_t, size_t>(is, filter.type, filter.range.low, filter.range.high, out);
        }
        else if (filter.type == "a16")
        {
            std::setlocale(LC_ALL, "C"); // isprint() -> ASCII
            find<char16_t, size_t>(is, filter.type, filter.range.low, filter.range.high, out);
        }
        else
            throw(unknown_type(filter.type));
    }
    return out;
}

std::vector<std::string> format_report(Report const& report)
{
    constexpr int addr_width = 8;
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
