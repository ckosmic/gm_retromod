#include "version.h"

void Version::Parse(int result[4], const std::string& input)
{
    std::istringstream parser(input);
    parser >> result[0];
    for (int idx = 1; idx < 4; idx++)
    {
        parser.get();
        parser >> result[idx];
    }
}

bool Version::LessThanVersion(const std::string& a, const std::string& b)
{
    int parsedA[4], parsedB[4];
    Parse(parsedA, a);
    Parse(parsedB, b);
    return std::lexicographical_compare(parsedA, parsedA + 4, parsedB, parsedB + 4);
}