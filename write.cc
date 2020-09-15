#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

template <typename T> void write(std::ostream& os, T value)
{
    std::cout << "number " << value << std::endl;
    os.write(reinterpret_cast<char*>(&value), sizeof value);
}

template <typename T> void write_string(std::ostream& os, const T* str, std::size_t length)
{
    os.write(reinterpret_cast<const char*>(str), (length + 1)*(sizeof str[0]));
    // for (size_t i = 0; os && str[i] != 0; ++i)
    //     os.write(reinterpret_cast<const char*>(str + i), sizeof str[i]);
}

// template<> void write(std::ostream& os, const char* str)
// {
//     std::cout << "string8 " << str << std::endl;
//     write_string(os, str);
// }
// template<> void write(std::ostream& os, const char16_t* str)
// {
//     std::cout << "string16 " << str << std::endl;
//     write_string(os, str);
// }
// template<> void write(std::ostream& os, const char32_t* str)
// {
//     std::cout << "string32 " << str << std::endl;
//     write_string(os, str);
// }

int main(int argc, char** argv)
{
    auto usage = "Usage: write [-u|-U] FILE";
    if (argc < 2)
    {
        std::cerr << usage;
        exit(-1);
    }
    std::string file = argv[1];
    std::size_t char_width = 1;
    if (argc > 2)
    {
        std::string opt = argv[1];
        if (opt == "-u")
            char_width = 2;
        else if (opt == "-U")
            char_width = 4;
        else
        {
            std::cerr << usage;
            exit(-1);
        }
        file = argv[2];
    }
    std::cout << char_width << std::endl;
    std::ofstream os(file);
    write(os, 1.23); // 3ff3 ae14 7ae1 47ae -> ae47 e17a 14ae f33f
    write(os, 432); // 1b0 -> b001 0000
    write(os, 0x00ff00ff00ff00ffL);
    switch (char_width)
    {
    case 1:
        write_string(os, "moo", 3); // moo -> 6d6f 6f
        write_string(os, "weeping willow", 14);
        break;
    case 2:
        write_string(os, u"moo", 3); // moo -> 6d6f 6f
        write_string(os, u"wæèþîñg wïllöw", 14);
        break;
    case 4:
        write_string(os, U"moo", 3); // moo -> 6d6f 6f
        write_string(os, U"weping willow", 14);
        break;
    default:
        std::cerr << usage;
        exit(-1);
        break;
    }
    write(os, -1); // ffff ffff
    write(os, 0);
    write(os, 0);
    write(os, 0);
    write(os, 1);
    return os.good();
}
