#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

enum class Color
{
    GREEN,
    YELLOW,
    RED,
    BLUE,
    WHITE
};
void printColored(const std::string &message, Color color);
void printColored(const std::string &message, Color color, bool newLine);

#endif