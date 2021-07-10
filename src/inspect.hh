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

#ifndef INSPECT_INSPECT_BINARY_INSPECT_HH_INCLUDED
#define INSPECT_INSPECT_BINARY_INSPECT_HH_INCLUDED

#include <cstdint>
#include <stdexcept>
#include <iosfwd>
#include <string>
#include <set>
#include <vector>

struct Range
{
    std::string low;
    std::string high;
    std::string min = "0"; // Exclude values within this much of zero.
};

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
using Report = std::multiset<Entry>;

/// @return all matches for all filters sorted by stream position.
Report inspect(std::istream& is, Spec const& spec);
/// Format the matches for display.
std::vector<std::string> format_report(Report const& report);

/// Exception raised when the range is empty.
struct unknown_type : public std::runtime_error
{
    unknown_type(std::string const& type)
        : runtime_error{"Unknown type: " + type}
    {}
};

/// Exception raised when the range is empty.
struct bad_range : public std::runtime_error
{
    bad_range(Range const& range)
        : runtime_error{"Low range > high (" + range.low + " > " + range.high + ")"}
    {}
};

#endif // INSPECT_INSPECT_BINARY_INSPECT_HH_INCLUDED
