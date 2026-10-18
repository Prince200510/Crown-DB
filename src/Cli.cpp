#include "Cli.h"
#include <cctype>
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#ifdef _WIN32
#include <windows.h>
#endif

CLI::CLI() : engine_(DatabaseEngine::create()), session_() {}

CLI::CLI(std::shared_ptr<DatabaseEngine> engine, SessionContext session)
    : engine_(std::move(engine)), session_(std::move(session)) {}

void CLI::start() {
    if (!session_.is_authenticated()) {
        std::cout << "Please login first\n";
        return;
    }

    const std::string version = "1.0";
    const std::string developer = "Prince Maurya";
    const std::string contact = "+91 9987742369";
    const std::string email = "princemaurya8879@gmail.com";

    std::cout << "Welcome to crownDB v1.0\n";
    std::cout << "Type 'crowndb --help' for list of commands.\n";
    std::cout << "Type 'EXIT;' to quit.\n";

    auto normalize = [](std::string text) {
        while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
            text.erase(text.begin());
        }
        while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
            text.pop_back();
        }
        if (!text.empty() && text.back() == ';') {
            text.pop_back();
        }
        std::string out;
        out.reserve(text.size());
        bool inSpace = false;
        for (unsigned char ch : text) {
            if (std::isspace(ch)) {
                if (!out.empty() && !inSpace) {
                    out.push_back(' ');
                }
                inSpace = true;
                continue;
            }
            out.push_back(static_cast<char>(std::toupper(ch)));
            inSpace = false;
        }
        while (!out.empty() && out.back() == ' ') {
            out.pop_back();
        }
        return out;
    };

    auto endsWithSemicolon = [](const std::string& text) {
        size_t end = text.size();
        while (end > 0 && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
            --end;
        }
        return end > 0 && text[end - 1] == ';';
    };

    bool running = true;
    std::map<std::string, std::function<void(std::string)>> commands;

     const auto addCommand = [&](std::string rawKey, std::function<void(std::string)> handler) {
        commands.emplace(normalize(std::move(rawKey)), std::move(handler));
    };

    const auto clearScreen = [&]() {
#ifdef _WIN32
        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
        if (console == INVALID_HANDLE_VALUE) return;
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (!GetConsoleScreenBufferInfo(console, &csbi)) return;
        const DWORD cellCount = static_cast<DWORD>(csbi.dwSize.X) * static_cast<DWORD>(csbi.dwSize.Y);
        DWORD count = 0;
        const COORD homeCoords = {0, 0};
        FillConsoleOutputCharacterA(console, ' ', cellCount, homeCoords, &count);
        FillConsoleOutputAttribute(console, csbi.wAttributes, cellCount, homeCoords, &count);
        SetConsoleCursorPosition(console, homeCoords);
#else
        std::cout << "\033[2J\033[H";
#endif
        std::cout.flush();
    };

    addCommand("crowndb --version", [&](std::string) {
        std::cout << "version :" << version << "\n";
    });

    addCommand("crowndb --v", [&](std::string) {
        std::cout << "version :" << version << "\n";
    });

    addCommand("exit", [&](std::string) {
        std::cout << "Goodbye!\n";
        running = false;
    });

    addCommand("cls", [&](std::string) {
        clearScreen();
    });

    addCommand("clear", [&](std::string) {
        clearScreen();
    });

    auto printHelp = [&](std::string) {
        std::cout << "Available commands:\n";
        std::cout << " - crowndb --version  : Show version information\n";
        std::cout << " - crowndb --v        : Show version information\n";
        std::cout << " - crowndb --help     : Show this help\n";
        std::cout << " - crowndb -h         : Show this help\n";
        std::cout << " - exit               : Exit the CLI\n";
        std::cout << "\nSQL commands (end with ';'):\n";
        std::cout << " - create database <name>;\n";
        std::cout << " - drop database <name>;\n";
        std::cout << " - use <name>;\n";
        std::cout << " - create table <table> (id int, name varchar(255), dob date);\n";
        std::cout << " - drop table <table>;\n";
        std::cout << " - truncate table <table>;\n";
        std::cout << " - alter table <table> add column <col> <type>;\n";
        std::cout << " - alter table <table> drop column <col>;\n";
        std::cout << " - insert into <table> values(1, 'prince', 2-2-2026);\n";
        std::cout << " - select * from <table>;\n";
        std::cout << " - select <col1>, <col2> from <table>;\n";
        std::cout << " - select * from <table> where <col> = <value>;\n";
        std::cout << " - select <col1>, <col2> from <table> where <col> = <value>;\n";
        std::cout << " - select count(*) from <table>;\n";
        std::cout << " - select sum(<int_col>) from <table>;\n";
        std::cout << " - select avg(<int_col>) from <table>;\n";
        std::cout << " - select min(<int_or_date_col>) from <table>;\n";
        std::cout << " - select max(<int_or_date_col>) from <table>;\n";
        std::cout << " - select <col>, max(<int_or_date_col>) from <table>;\n";
        std::cout << "===============================================================\n";
        std::cout << "Developer Information:\n";
        std::cout << "Developer name: " << developer << "\n";
        std::cout << "Contact: " << contact << "\n";
        std::cout << "Email: " << email << "\n";
    };

    addCommand("crowndb --help", printHelp);
    addCommand("crowndb --h", printHelp);
    addCommand("crowndb -h", printHelp);
    addCommand("help", printHelp);

    std::string input;
    std::string statement;

    while (running) {
        if (statement.empty()) {
            std::cout << "crownDB[" << session_.user();
            if (session_.has_active_database()) {
                std::cout << ":" << session_.current_database();
            }
            std::cout << "]> ";
        } else {
            std::cout << "           -> ";
        }

        if (!std::getline(std::cin, input)) {
            break;
        }

        if (statement.empty()) {
            std::string single = normalize(input);
            auto it = commands.find(single);
            if (it != commands.end()) {
                it->second(input);
                continue;
            }
        }

        if (!statement.empty()) {
            statement += " ";
        }
        statement += input;

        if (!endsWithSemicolon(statement)) {
            continue;
        }

        std::string fullSql = std::move(statement);
        statement.clear();

        std::string checkExit = normalize(fullSql);
        if (checkExit == "EXIT" || checkExit == "QUIT") {
            std::cout << "Goodbye!\n";
            running = false;
            break;
        }

        auto start_time = std::chrono::high_resolution_clock::now();
        QueryResult result = engine_->execute(session_, fullSql);
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end_time - start_time;

        if (!result.is_ok()) {
            std::cerr << result.error().message() << "\n";
        } else {
            std::cout << result.to_string();
            std::cout << "(" << elapsed.count() << " ms)\n";
        }
    }
}
