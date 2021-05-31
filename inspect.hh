#ifndef INSPECT_INSPECT_BINARY_INSPECT_HH_INCLUDED
#define INSPECT_INSPECT_BINARY_INSPECT_HH_INCLUDED

#include <cstdint>
#include <stdexcept>
#include <iosfwd>
#include <string>
#include <vector>

/// The type for range limits. Must be large enough to specify the full range of the
/// widest integer type.
using range_t = int64_t;
/// A min/max pair.
using Range = std::pair<range_t, range_t>;

/// The user-specified range for a specific type
struct Filter
{
    std::string type;
    Range range;
};

/// The complete specification about what to look for.
using Spec = std::vector<Filter>;

/// Information about a match in the binary file.
struct Entry
{
    std::streamoff address = -1;
    std::string value;
    std::string type;
};

/// All of the matches found.
using Report = std::vector<Entry>;

/// @return all matches for all filters sorted by stream position.
Report inspect(std::istream& is, const Spec& spec);
/// Format the matches for display.
std::vector<std::string> format_report(const Report& report);

/// Exception raised when the range is empty.
struct unknown_type : public std::runtime_error
{
    unknown_type(const std::string& type)
        : runtime_error{"Unknown type: " + type}
    {}
};

/// Exception raised when the range is empty.
struct bad_range : public std::runtime_error
{
    bad_range(range_t low, range_t high)
        : runtime_error{"Low range > high ("
        + std::to_string(low) + " > " + std::to_string(high) + ")"}
    {}
};

#endif // INSPECT_INSPECT_BINARY_INSPECT_HH_INCLUDED
