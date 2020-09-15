#include "inspect.hh"

#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

class bad_type : public std::exception
{
public:
    bad_type(const std::string& type)
        : m_message(std::string("Unrecognized type: ") + type) {}
    virtual const char* what() const noexcept override {
        return m_message.c_str(); }
private:
    std::string m_message;
};

std::map<std::string, Range> default_ranges = {
    {"f64", {-6, 6}},
    {"f32", {-6, 6}},
    {"i64", {-1000, 1000}},
    {"i32", {-1000, 1000}},
    {"i16", {-1000, 1000}},
    {"s32", {-3, 64}},
    {"s16", {-3, 64}},
    {"s8",  {-3, 64}},
};

auto get_range = [](const std::string& str) {
    range_t low = 0;
    range_t high = 0;
    char sep;
    std::istringstream is(str);
    is >> low >> sep >> high;
    if (low >= high)
        throw std::invalid_argument("Low range >= high");
    return std::make_pair(low, high);
};

Spec parse_spec_opts(const std::vector<std::string>& opts)
{
    if (opts.empty())
        return {{"i32", {-1000, 1000}},
                {"f64", {-6, 6}},
                {"s8",  {3, 64}}};

    Spec spec;
    Filter f;
    for (const auto& opt : opts)
    {
        if (opt.find(':') == std::string::npos)
        {
            f.type = opt.substr(1);
            // Throw std::out_of_range if the type is not recognized
            try
            {
                f.range = default_ranges.at(f.type);
            }
            catch(const std::out_of_range& e)
            {
                throw bad_type(f.type);
            }
            spec.push_back(f);
            continue;
        }
        spec.back().range = get_range(opt);
    }
    return spec;
}

int main(int argc, char** argv)
{
    std::string usage =
        "Usage: inspect FILE [SPEC]\n"
        "\n"
        "  SPEC: -<type> [<low>:<high>]\n"
        "  <type>: f64|f32|i64|i32|i16|s8 (double, float, n-bit int,\n"
        "          string)\n"
        "  <low>, <high>: Allowed range, integer.\n"
        "                 Exponents for f\n"
        "                 Values for i\n"
        "                 String length for s\n"
        "\n"
        "Example: inspect moo -f32 -6:6 -i32 -10:100 -s\n"
        "Show floats from 10e-6 to 10e6, ints from -10 to 100, and\n"
        "strings with the default range: 3 to 64 characters.\n";

    if (argc < 2)
    {
        std::cerr << usage;
        exit(-1);
    }

    std::vector<std::string> opts;
    for (int i = 2; i < argc; ++i)
        opts.push_back(argv[i]);

    Spec spec;
    try
    {
        spec = parse_spec_opts(opts);
    }
    catch(const bad_type& e)
    {
        std::cerr << "Error: " << e.what() << "\n\n"
                  << usage;
        exit(-1);
    }
    catch(const std::invalid_argument& e)
    {
        std::cerr << "Error, bad range: " << e.what() << "\n\n"
                  << usage;
        exit(-1);
    }
    auto is = std::ifstream(argv[1]);
    for (const auto& line : format_report(inspect(is, spec)))
        std::cout << line << std::endl;
    return 0;
}
