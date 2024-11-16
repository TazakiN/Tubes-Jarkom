#include <iostream>
#include "utils.hpp"

void printColored(const std::string &message, Color color)
{
    printColored(message, color, true);
}

void printColored(const std::string &message, Color color, bool newLine)
{
    switch (color)
    {
    case Color::GREEN:
        std::cout << "\033[32m"; // Green
        break;
    case Color::YELLOW:
        std::cout << "\033[33m"; // Yellow
        break;
    case Color::RED:
        std::cout << "\033[31m"; // Red
        break;
    case Color::BLUE:
        std::cout << "\033[34m"; // Blue
        break;
    case Color::WHITE:
        std::cout << "\033[37m"; // White
        break;
    }

    std::cout << message;

    if (newLine)
    {
        std::cout << std::endl;
    }
}