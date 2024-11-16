#include "server.hpp"
#include <iostream>
#include <fstream>
#include "utils.hpp"

Server::Server(const string &ip, int port)
{
    connection = new TCPSocket(ip, port);
}

void Server::run()
{
    printColored("[i] Node is now a sender", Color::BLUE);
    int mode = inputMethodMenu();

    string filePath;

    if (mode == 1)
    {
        string userInput = getUserInput(); // Input manual disimpan ke file
        if (!userInput.empty())
        {
            filePath = saveToFile(userInput);
            if (!filePath.empty())
            {
                printColored("[+] User input saved to file.", Color::GREEN);
            }
        }
        else
        {
            printColored("[!] No input provided. Exiting...", Color::RED);
            return;
        }
    }
    else if (mode == 2) // File eksternal
    {
        filePath = getFilePath();
        if (filePath.empty())
        {
            printColored("[!] Invalid file path. Exiting...", Color::RED);
            return;
        }
    }

    // TODO:  kirim file ke client (file ada di filePath)

    printColored("[i] Listening to the broadcast port for clients.", Color::BLUE);
}

void Server::handleMessage(void *buffer)
{
    // Implementasi dari handleMessage()
    // Tambahkan logika penanganan pesan di sini
}

int Server::inputMethodMenu()
{
    printColored("[?] Please choose the sending mode:", Color::YELLOW);
    printColored("[?] 1. User Input", Color::YELLOW);
    printColored("[?] 2. File Input", Color::YELLOW);
    printColored("[?] Enter your choice: ", Color::YELLOW, false);

    int choice;
    cin >> choice;
    cin.ignore();
    return choice;
}

string Server::getUserInput()
{
    printColored("[?] Please enter your input: ", Color::YELLOW, false);
    string input;
    getline(cin, input);
    return input;
}

string Server::getFilePath()
{
    printColored("[?] Please enter the file path: ", Color::YELLOW, false);
    string filePath;
    getline(cin, filePath);
    return filePath;
}

string Server::saveToFile(const string &data)
{
    string fileName = "user_input.txt";
    ofstream file(fileName);
    if (!file)
    {
        printColored("[!] Failed to create file for user input.", Color::RED);
        return "";
    }
    file << data;
    file.close();
    return fileName;
}
