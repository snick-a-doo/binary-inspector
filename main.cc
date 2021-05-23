#include "inspect.hh"

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

#include <cstring>
#include <exception>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <map>
#include <numeric>
#include <span>
#include <sstream>
#include <string>
#include <vector>

/// Exception raised when the range isn't in the expected format.
struct bad_format : public std::runtime_error
{
    bad_format(const std::string& arg)
        : runtime_error{"Range format should be <low>:<high> (" + arg + ")"}
    {}
};

/// Exception raised when the range is empty.
struct bad_range : public std::runtime_error
{
    bad_range(range_t low, range_t high)
        : runtime_error{"Low range >= high ("
        + std::to_string(low) + " >= " + std::to_string(high) + ")"}
    {}
};

/// Exception raised when a file name isn't given.
struct missing_file : public std::runtime_error
{
    missing_file()
        : std::runtime_error{"A file name to inspect was not given."}
    {}
};

/// The ranges used if one isn't specified.
const std::map<std::string, Range> default_ranges = {
    {"f64", {-6, 6}}, // exponent
    {"f32", {-6, 6}},
    {"i64", {-1000, 1000}}, // value
    {"i32", {-1000, 1000}},
    {"i16", {-1000, 1000}},
    {"s16", {3, 64}}, // string length
    {"s8",  {3, 64}},
};

/// The ranges used if no ranges are specified.
const Spec default_spec = {
    {"i32", {-1000, 1000}},
    {"f64", {-6, 6}},
    {"s8",  {3, 64}},
};

/// Parse the range specification and return a range object or throw.
Range get_range(const std::string& str)
{
    range_t low = 0;
    range_t high = 0;
    char sep;
    std::istringstream is(str);
    is >> low >> sep >> high;
    if (sep != ':' || !is)
        throw bad_format(str);
    if (low >= high)
        throw bad_range(low, high);
    return {low, high};
};

/// @return The string representation of a collection of range filters.
std::string to_string(const Spec& spec)
{
    auto append = [](const auto& s1, const auto& p2) {
        std::ostringstream os;
        os << s1 << "--" << p2.type << '='
           << p2.range.first << ':' << p2.range.second << ' ';
        return os.str();
    };
    return std::accumulate(spec.begin(), spec.end(), std::string(), append);
}

/// The help message.
const std::string usage =
    "Usage: inspect [options] file\n"
    "\n"
    "  -d --f64=[range] show double-precision floats.\n"
    "  -f --f32=[range] show single-precision floats.\n"
    "  -l --i64=[range] show 64-bit integers.\n"
    "  -i --i32=[range] show 32-bit integers.\n"
    "  -s --i16=[range] show 16-bit integers.\n"
    "  -u --s16=[range] show wide ASCII strings.\n"
    "  -z --s8=[range]  show ASCII strings.\n"
    "\n"
    "Range is given as <low>:<high>. For floats, <low> and <high> are exponents,\n"
    "for integers they're values, for strings they're lengths.\n"
    "\n"
    "With no options, the behavior is the same as\n"
    + to_string(default_spec)
    + '\n';

/// Parse the command line.
/// @return A pair of the name of the file to inspect and the range filters.
std::pair<std::string, Spec> parse_args(int argc, char** argv)
{
    Spec spec;
    option options[] = {
        {"f64", optional_argument, nullptr, 'd'},
        {"f32", optional_argument, nullptr, 'f'},
        {"i64", optional_argument, nullptr, 'l'},
        {"i32", optional_argument, nullptr, 'i'},
        {"i16", optional_argument, nullptr, 's'},
        {"s16", optional_argument, nullptr, 'u'},
        {"s8", optional_argument, nullptr, 'z'},
        {"help", no_argument, nullptr, 'h'},
        {0, 0, 0, 0}};

    auto add_filter = [&](const std::string& opt) {
        spec.push_back(Filter{opt, ::optarg ? get_range(::optarg) : default_ranges.at(opt)});
    };

    // getopt doesn't expect to be called multiple times, but it is if tests are
    // run. Make sure the state initialized.
    ::optopt = 0;
    ::optind = 0;
    ::optarg = nullptr;
    while (true)
    {
        int index;
        int c = getopt_long(argc, argv, "d::f::l::i::s::u::z::", options, &index);
        if (c == -1)
            break;
        switch (c)
        {
        case 'd':
            add_filter("f64");
            break;
        case 'f':
            add_filter("f32");
            break;
        case 'l':
            add_filter("i64");
            break;
        case 'i':
            add_filter("i32");
            break;
        case 's':
            add_filter("i16");
            break;
        case 'u':
            add_filter("s16");
            break;
        case 'z':
            add_filter("s8");
            break;
        case 'h':
            std::cerr << usage;
            exit(0);
        default:
            break;
        }
    }

    if (!argv[::optind])
        throw(missing_file());
    return {argv[::optind], spec.empty() ? default_spec : spec};
}

// Entry point
int main(int argc, char** argv)
{
    // Run runtime unit tests.
    doctest::Context test_context;
    test_context.applyCommandLine(argc, argv);
    int exit_status = test_context.run();
    if (test_context.shouldExit())
        return exit_status;

    try
    {
        auto [file, spec] = parse_args(argc, argv);
        auto is = std::ifstream(file);
        for (const auto& line : format_report(inspect(is, spec)))
            std::cout << line << std::endl;
    }
    catch(const std::runtime_error& e)
    {
        std::cerr << "Error: " << e.what() << "\n\n" << usage;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

// Runtime unit tests

TEST_CASE("parse range")
{
    CHECK(get_range("1:2") == Range {1, 2});
    CHECK_THROWS_AS(get_range("1-2"), bad_format);
    CHECK_THROWS_AS(get_range("1/2"), bad_format);

    CHECK(get_range("-2:-1") == Range {-2, -1});
    CHECK_THROWS_AS(get_range("-2--1"), bad_format);
    CHECK_THROWS_AS(get_range("-2 - -1"), bad_format);
    CHECK_THROWS_AS(get_range("-2/-1"), bad_format);

    CHECK(get_range("-2:1") == Range {-2, 1});
    CHECK_THROWS_AS(get_range("-2-1"), bad_format);
    CHECK_THROWS_AS(get_range("-2/1"), bad_format);

    CHECK(get_range("-999999999999:999999999999")
          == Range {-999'999'999'999L, 999'999'999'999L});

    CHECK_THROWS_AS(get_range(""), bad_format);
    CHECK_THROWS_AS(get_range("22:22"), bad_range);
    CHECK_THROWS_AS(get_range("22:3"), bad_range);
    CHECK_THROWS_AS(get_range("22:"), bad_format);
    CHECK_THROWS_AS(get_range(":3"), bad_format);
    CHECK_THROWS_AS(get_range("22"), bad_format);
}

TEST_CASE("spec to string")
{
    CHECK(to_string(Spec()) == "");
    CHECK(to_string(default_spec) == "--i32=-1000:1000 --f64=-6:6 --s8=3:64 ");
}

bool operator==(const Filter& f1, const Filter& f2)
{
    return f1.type == f2.type && f1.range == f2.range;
}

TEST_CASE("args")
{
    auto parse = [](std::vector<std::string>&& args) {
        auto argv = std::make_unique<char*[]>(args.size() + 1);
        // Arguments need to be stored somewhere because they're passed as non-const char*
        // to porse_args(). That's because getopt() requires non-const char arrays.
        std::string appname = "app";
        argv[0] = appname.data();
        for (size_t i = 0; i < args.size(); ++i)
            argv[i+1] = args[i].data();
        return parse_args(args.size()+1, argv.get());
    };

    std::string file = "file";
    auto result = [&file](const Spec& spec) {
        return std::make_pair(file, spec);
    };

    CHECK_THROWS_AS(parse({}), missing_file);
    CHECK_THROWS_AS(parse({"--f64"}), missing_file);
    CHECK(parse({file}) == result(default_spec));

    CHECK(parse({file, "-d"}) == result({{"f64", default_ranges.at("f64")}}));
    CHECK(parse({file, "--f64"}) == result({{"f64", default_ranges.at("f64")}}));
    CHECK(parse({file, "-d-3:9"}) == result({{"f64", {-3, 9}}}));
    CHECK(parse({file, "--f64=-3:9"}) == result({{"f64", {-3, 9}}}));
    CHECK(parse({file, "-f"}) == result({{"f32", default_ranges.at("f32")}}));
    CHECK(parse({file, "--f32"}) == result({{"f32", default_ranges.at("f32")}}));
    CHECK(parse({file, "-f-3:9"}) == result({{"f32", {-3, 9}}}));
    CHECK(parse({file, "--f32=-3:9"}) == result({{"f32", {-3, 9}}}));
    CHECK(parse({file, "-l"}) == result({{"i64", default_ranges.at("i64")}}));
    CHECK(parse({file, "--i64"}) == result({{"i64", default_ranges.at("i64")}}));
    CHECK(parse({file, "--i64=-3:9"}) == result({{"i64", {-3, 9}}}));
    CHECK(parse({file, "-l-3:9"}) == result({{"i64", {-3, 9}}}));
    CHECK(parse({file, "-i"}) == result({{"i32", default_ranges.at("i32")}}));
    CHECK(parse({file, "--i32"}) == result({{"i32", default_ranges.at("i32")}}));
    CHECK(parse({file, "-i-3:9"}) == result({{"i32", {-3, 9}}}));
    CHECK(parse({file, "--i32=-3:9"}) == result({{"i32", {-3, 9}}}));
    CHECK(parse({file, "-s"}) == result({{"i16", default_ranges.at("i16")}}));
    CHECK(parse({file, "--i16"}) == result({{"i16", default_ranges.at("i16")}}));
    CHECK(parse({file, "-s-3:9"}) == result({{"i16", {-3, 9}}}));
    CHECK(parse({file, "--i16=-3:9"}) == result({{"i16", {-3, 9}}}));
    CHECK(parse({file, "-z"}) == result({{"s8", default_ranges.at("s8")}}));
    CHECK(parse({file, "--s8"}) == result({{"s8", default_ranges.at("s8")}}));
    CHECK(parse({file, "-z 3:9"}) == result({{"s8", {3, 9}}}));
    CHECK(parse({file, "--s8=3:9"}) == result({{"s8", {3, 9}}}));
    CHECK(parse({file, "--s8=-3:9"}) == result({{"s8", {-3, 9}}}));

    CHECK_THROWS_AS(parse({file, "--i32=0:-25"}), bad_range);
    CHECK_THROWS_AS(parse({file, "--i32=0-25"}), bad_format);
}
