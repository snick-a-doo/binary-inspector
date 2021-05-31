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

/// Dummy types for template function resolution.
struct dummy_ascii8_t{};
struct dummy_ascii16_t{};

/// @return True if the argument is an ASCII character, i.e. printable in the C locale.
bool is_ascii(unsigned char c)
{
    std::setlocale(LC_ALL, "C");
    return std::isprint(c);
}

/// @return True if the argument is in the Latin-1 extended ASCII character set,
/// i.e. printable in the ISO-88591 locale.
bool is_latin(unsigned char c)
{
    std::setlocale(LC_ALL, "en_US.iso88591");
    return std::isprint(c);
}

template <typename T>
Entry read_number(std::istream& is, std::function<bool(T)> accept)
{
    while(is)
    {
        auto start = is.tellg();
        T value;
        is.read(reinterpret_cast<char*>(&value), sizeof value);
        // Advance 1 byte
        is.seekg(start + std::istream::pos_type(1));
        if (accept(value))
            return {start, std::to_string(value), ""};
    }
    return {};
};

template <typename T>
Entry read_string(std::istream& is, const Range& range, std::function<bool(T)> accept)
{
    if (!is)
        return {};
    // Find the bounds of the next null-terminated ASCII string that's within the length
    // limits.
    std::string out;
    auto start = is.tellg();
    while (is)
    {
        T c;
        is.read(reinterpret_cast<char*>(&c), sizeof c);
        auto c_low = static_cast<unsigned char>(c);
        auto len = static_cast<range_t>(out.length());
        // Check for end of a long enough string.
        if ((c_low == '\0' || c_low == '\t' || c_low == '\n' || c_low == '\r')
            && len >= range.first)
            return {start, out, ""};
        // Add a good character to the string.
        if (accept(c_low) && c_low == c && len < range.second)
            out.push_back(c_low);
        else
        {
            // Bad character or too long.
            // Consume any remaining characters from an over-long string.
            while (accept(c_low) && c_low == c)
            {
                start = is.tellg();
                is.read(reinterpret_cast<char*>(&c), sizeof c);
                c_low = static_cast<unsigned char>(c);
            }
            // Start looking for the next string at the next byte. This requires rewinding
            // the stream if T is wider than 1 byte.
            start += std::istream::pos_type(1);
            is.seekg(start);
            out.clear();
        }
    }
    return {};
}

template <typename T>
Entry read_next(std::istream& is, const Range& range)
{
    return read_number<T>(is, [range](T v) {
        return v >= range.first && v <= range.second; });
}

auto is_in_exp_range = [](auto x, const Range& range) {
    return std::abs(x) >= std::pow(10, range.first)
        && std::abs(x) <= std::pow(10, range.second)
        && std::isfinite(x);
};

using namespace std::placeholders;

template <> Entry
read_next<double>(std::istream& is, const Range& range)
{
    return read_number<double>(is, std::bind(is_in_exp_range, _1, range));
}

template <> Entry
read_next<float>(std::istream& is, const Range& range)
{
    return read_number<float>(is, std::bind(is_in_exp_range, _1, range));
}

template <>
Entry read_next<dummy_ascii8_t>(std::istream& is, const Range& range)
{
    return read_string<char8_t>(is, range, is_ascii);
}

template <>
Entry read_next<dummy_ascii16_t>(std::istream& is, const Range& range)
{
    return read_string<char16_t>(is, range, is_ascii);
}

template <>
Entry read_next<char8_t>(std::istream& is, const Range& range)
{
    return read_string<char8_t>(is, range, is_latin);
}

template <>
Entry read_next<char16_t>(std::istream& is, const Range& range)
{
    return read_string<char16_t>(is, range, is_latin);
}

/// Get everything in the stream that matches the given filter.
template <typename T> Report find(std::istream& is, const Filter& filter)
{
    Report out;
    while (true)
    {
        auto entry = read_next<T>(is, filter.range);
        if (!is)
            return out;
        entry.type = filter.type;
        out.push_back(entry);
    }
}

Report inspect(std::istream& is, const Spec& spec)
{
    Report out;
    const auto start = is.tellg();
    for (const auto& filter : spec)
    {
        if (filter.range.first > filter.range.second)
            throw(bad_range{filter.range.first, filter.range.second});
        Report sub;
        is.clear(); // Recover from EOF in previous iteration.
        is.seekg(start);
        if (filter.type == "f64")
            sub = find<double>(is, filter);
        else if (filter.type == "f32")
            sub = find<float>(is, filter);
        else if (filter.type == "i64")
            sub = find<int64_t>(is, filter);
        else if (filter.type == "i32")
            sub = find<int32_t>(is, filter);
        else if (filter.type == "i16")
            sub = find<int16_t>(is, filter);
        else if (filter.type == "s8")
            sub = find<char8_t>(is, filter);
        else if (filter.type == "s16")
            sub = find<char16_t>(is, filter);
        else if (filter.type == "a8")
            sub = find<dummy_ascii8_t>(is, filter);
        else if (filter.type == "a16")
            sub = find<dummy_ascii16_t>(is, filter);
        else
            throw(unknown_type(filter.type));
        out.insert(out.end(), sub.begin(), sub.end());
    }
    // Sort entries by stream position.
    std::sort(out.begin(), out.end(),
              [](const Entry& a, const Entry& b) { return a.address < b.address; });
    return out;
}

std::vector<std::string> format_report(const Report& report)
{
    constexpr int addr_width = 8;
    std::vector<std::string> out;
    Entry last_entry;
    for (const auto& entry : report)
    {
        const auto& [addr, value, type] = entry;
        std::ostringstream pos;
        pos << std::setfill('0') << std::setw(addr_width) << std::hex << addr;
        auto lsd = pos.str()[7];
        // If a value occurs multiple times on the same 16-byte line, show just one entry
        // in the report but mark the LSD of each address.
        if (addr >> 4 == last_entry.address >> 4
            && type == last_entry.type
            && value == last_entry.value)
        {
            out.back()[addr_width + (addr & 0xf)] = lsd;
            continue;
        }
        // Format the line.
        std::ostringstream line;
        // Don't repeat the address if it's the same up to the LSD.
        line << (last_entry.address != -1 && last_entry.address >> 4 == addr >> 4
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
