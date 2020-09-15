#ifndef INSPECT_HH
#define INSPECT_HH

#include <cstdint>
#include <iosfwd>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using range_t = std::int64_t;
using Range = std::pair<range_t, range_t>;

struct Filter
{
    std::string type;
    Range range;
};

using Spec = std::vector<Filter>;

struct Entry
{
    std::streamoff address;
    std::string value;
    std::string type;
};
using Report = std::vector<Entry>;

Report inspect(std::istream& is, const Spec& spec);
std::vector<std::string> format_report(const Report& report);

#endif
