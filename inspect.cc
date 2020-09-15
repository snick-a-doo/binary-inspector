#include "inspect.hh"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <istream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

auto read_number = [](std::istream& is, auto& value, auto reject)
{
    assert(is);
    auto start = is.tellg();
    do
    {
        start = is.tellg();
        is.read(reinterpret_cast<char*>(&value), sizeof value);
        is.seekg(start + std::istream::pos_type(1));
    }
    while (is && reject(value));
    return Entry{start, std::to_string(value), ""};
};

template <typename T> Entry read_next(std::istream& is, T& value, Range range)
{
    return read_number(is, value, [range](T v) {
        return v < range.first || v > range.second; });
}

Entry read_next(std::istream& is, double& value, Range range)
{
    return read_number(is, value, [range](double v) {
        return std::abs(v) < std::pow(10, range.first)
            || std::abs(v) > std::pow(10, range.second)
            || !std::isfinite(v);
    });
}

Entry read_next(std::istream& is, float& value, Range range)
{
    return read_number(is, value, [range](float v) {
        return std::abs(v) < std::pow(10, range.first)
            || std::abs(v) > std::pow(10, range.second)
            || !std::isfinite(v);
    });
}

template <typename T>
Entry read_next(std::istream& is, std::basic_string<T>& str, Range range)
{
    assert(is);
    std::optional<std::istream::pos_type> start;
    std::optional<std::istream::pos_type> end;
    // Find the bounds of the next null-terminated ASCII string that's within the length
    // limits.
    while (is && !end)
    {
        auto pos = is.tellg();
        T str_char;
        is.read(reinterpret_cast<char*>(&str_char), sizeof str_char);
        auto c = static_cast<unsigned char>(str_char);
        if (start && c == '\0')
        {
            auto len = static_cast<int>(pos - *start);
            if (len < range.first || len > range.second)
                start.reset();
            else
                end = pos;
        }
        else if (c < ' ' || c == 0x7f || c > 0xfc) // Not in Latin-1
            start.reset();
        else if (!start)
            start = pos;
    }
    // If we didn't find the end of a string we must be at the end of the stream.
    assert(end || !is);
    // If we found the end, we must have found the start.
    assert(!end || start);
    if (!end)
        return {0, "", ""};
    is.clear(); // Recover from possible EOF
    // Rewind to the start of the string and read in the characters.
    is.seekg(*start);
    std::string out;
    while (is)
    {
        T str_char;
        is.read(reinterpret_cast<char*>(&str_char), sizeof str_char);
        char c = static_cast<char>(str_char);
        if (c == '\0')
            break;
        out += c;
    }
    return {*start, out, ""};
}

template <typename T> Report find(std::istream& is, T value, const Filter& f)
{
    Report out;
    while (is)
    {
        auto entry = read_next(is, value, f.range);
        if (!is)
            break;
        entry.type = f.type;
        out.push_back(entry);
    }
    return out;
}

Report inspect(std::istream& is, const Spec& spec)
{
    Report out;
    const std::istream::pos_type start = is.tellg();
    for (const auto& s : spec)
    {
        Report sub;
        is.clear(); // Recover from EOF in previous iteration.
        is.seekg(start);
        if (s.type == "f64")
            sub = find(is, double(0), s);
        else if (s.type == "f32")
            sub = find(is, float(0), s);
        else if (s.type == "i64")
            sub = find(is, std::int64_t(0), s);
        else if (s.type == "i32")
            sub = find(is, std::int32_t(0), s);
        else if (s.type == "i16")
            sub = find(is, std::int16_t(0), s);
        else if (s.type == "s8")
            sub = find(is, std::string(), s);
        else if (s.type == "s16")
            sub = find(is, std::basic_string<char16_t>(), s);
        else if (s.type == "s32")
            sub = find(is, std::basic_string<char32_t>(), s);
        else
            assert(false);
        out.insert(out.end(), sub.begin(), sub.end());
    }
    // Sort entries by stream position.
    std::sort(out.begin(), out.end(),
              [](const Entry& a, const Entry& b) { return a.address < b.address; });
    return out;
}

std::vector<std::string> format_report(const Report& report)
{
    std::vector<std::string> out;
    std::optional<unsigned int> last_addr;
    std::string last_type;
    std::string last_value;
    for (const auto& entry : report)
    {
        auto [addr, value, type] = entry;
        std::ostringstream pos;
        pos << std::setfill('0') << std::setw(8) << std::hex << addr;
        auto lsd = pos.str()[7];
        if (type == last_type && value == last_value && lsd != '0')
        {
            // Add byte to the previous line.
            std::size_t i = 8 + addr % 16;
            out.back()[i] = lsd;
            continue;
        }
        auto fill = addr % 0x10;
        std::ostringstream line;
        line << (last_addr && *last_addr >> 4 == addr >> 4
                 ? std::string(7, ' ') : pos.str().substr(0, 7))
             << std::string(fill + 1, ' ')
             << lsd
             << std::string(0x11 - fill, ' ')
             << type << std::string(4 - type.size(), ' ')
             << std::dec << value;
        out.push_back(line.str());
        last_addr = addr;
        last_type = type;
        last_value = value;
    }
    return out;
}
