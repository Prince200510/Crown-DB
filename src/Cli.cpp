#include "Cli.h"
#include "EngineAPI.h"

#include <cctype>
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

void CLI::start() {
    EngineAPI engine_api;
    const std::string version = "1.0";
    const std::string developer = "Prince Maurya";
    const std::string contact = "+91 9987742369";
    const std::string email = "princemaurya8879@gmail.com";

    std::cout << "Welcome to crownDB v1.0\n";
    std::cout << "Type 'register' to create a new user or 'login' to access an existing account:\n";
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

    auto cleanInput = [](std::string text) {
        while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
            text.erase(text.begin());
        }
        while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
            text.pop_back();
        }
        if (!text.empty() && text.back() == ';') {
            text.pop_back();
        }
        while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
            text.pop_back();
        }
        return text;
    };

    auto splitWords = [](const std::string& text) {
        std::vector<std::string> words;
        std::string current;
        for (unsigned char ch : text) {
            if (std::isspace(ch)) {
                if (!current.empty()) {
                    words.push_back(std::move(current));
                    current.clear();
                }
                continue;
            }
            current.push_back(static_cast<char>(ch));
        }
        if (!current.empty()) {
            words.push_back(std::move(current));
        }
        return words;
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

    std::map<std::string, std::function<void(const std::vector<std::string>& args, const std::string& rawInput)>> argCommands;
    size_t maxArgCommandWords = 0;

    const auto addArgCommand = [&](std::string rawKey, std::function<void(const std::vector<std::string>& args, const std::string& rawInput)> handler) {
        const std::string key = normalize(std::move(rawKey));
        maxArgCommandWords = std::max(maxArgCommandWords, splitWords(key).size());
        argCommands.emplace(key, std::move(handler));
    };

    const auto clearScreen = [&]() {
        #ifdef _WIN32
                HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
                if (console == INVALID_HANDLE_VALUE) {
                    return;
                }
            
                CONSOLE_SCREEN_BUFFER_INFO csbi;
                if (!GetConsoleScreenBufferInfo(console, &csbi)) {
                    return;
                }
            
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

    addArgCommand("create database", [&](const std::vector<std::string>& args, const std::string&) {
        if (args.empty() || args[0].empty()) {
            std::cout << "Usage: create database <database_name>;\n";
            return;
        }

        const std::string& dbName = args[0];
        const auto start = std::chrono::steady_clock::now();
        const bool created = engine_api.create_database(dbName);
        const auto end = std::chrono::steady_clock::now();
        const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        if (created) {
            std::cout << "Query OK, database created successfully in " << elapsedMs << " ms\n";
        } else {
            std::cout << "Database already exists (" << elapsedMs << " ms)\n";
        }
    });

    addArgCommand("drop database", [&](const std::vector<std::string>& args, const std::string&) {
        if (args.empty() || args[0].empty()) {
            std::cout << "Usage: drop database <database_name>;\n";
            return;
        }

        const std::string& dbName = args[0];
        const auto start = std::chrono::steady_clock::now();
        const bool dropped = engine_api.drop_database(dbName);
        const auto end = std::chrono::steady_clock::now();
        const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        if (dropped) {
            std::cout << "Query OK, database dropped successfully in " << elapsedMs << " ms\n";
        } else {
            std::cout << "Database does not exist (" << elapsedMs << " ms)\n";
        }
    });

    addArgCommand("use", [&](const std::vector<std::string>&, const std::string& rawInput) {
        if (!endsWithSemicolon(rawInput)) {
            std::cout << "Statement must end with ';'\n";
            return;
        }

        std::string stmt = rawInput;
        if (!stmt.empty() && stmt.back() == ';') {
            stmt.pop_back();
        }

        auto ltrim = [](std::string& s) {
            while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
                s.erase(s.begin());
            }
        };
        auto rtrim = [](std::string& s) {
            while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
                s.pop_back();
            }
        };

        ltrim(stmt);
        if (stmt.size() < 4) {
            std::cout << "Usage: use <database_name>;\n";
            return;
        }

        std::string name = stmt.substr(4);
        ltrim(name);
        rtrim(name);

        if (name.empty()) {
            std::cout << "Usage: use <database_name>;\n";
            return;
        }

        if (engine_api.use_database(name)) {
            std::cout << "Database changed\n";
        } else {
            std::cout << "Invalid or corrupted database\n";
        }
    });

    std::string input;
    std::string statement;
    while (running) {
        if (statement.empty()) {
            std::cout << "crownDB Client> ";
        } else {
            std::cout << "           -> ";
        }

        if (!std::getline(std::cin, input)) {
            break;
        }

        if (statement.empty()) {
            const std::string rawCleanLine = cleanInput(input);
            const std::string keyLine = normalize(rawCleanLine);
            auto it = commands.find(keyLine);
            if (it != commands.end()) {
                it->second(input);
                continue;
            }
        }

        if (!statement.empty()) {
            statement.push_back(' ');
        }
        statement += input;

        if (!endsWithSemicolon(statement)) {
            continue;
        }

        const std::string rawStatement = statement;
        const std::string rawClean = cleanInput(statement);
        const std::string key = normalize(rawClean);
        statement.clear();

        auto it = commands.find(key);
        if (it != commands.end()) {
            it->second(rawClean);
            continue;
        }

        bool handled = false;
        if (!argCommands.empty()) {
            const auto keyWords = splitWords(key);
            const auto rawWords = splitWords(rawClean);
            const size_t limit = std::min(keyWords.size(), maxArgCommandWords);
            for (size_t n = limit; n >= 1; --n) {
                std::string cmdKey;
                for (size_t i = 0; i < n; ++i) {
                    if (i != 0) {
                        cmdKey.push_back(' ');
                    }
                    cmdKey += keyWords[i];
                }

                auto argIt = argCommands.find(cmdKey);
                if (argIt != argCommands.end()) {
                    std::vector<std::string> args;
                    if (rawWords.size() > n) {
                        args.assign(rawWords.begin() + static_cast<std::ptrdiff_t>(n), rawWords.end());
                    }
                    argIt->second(args, rawStatement);
                    handled = true;
                    break;
                }

                if (n == 1) {
                    break;
                }
            }
        }

        if (!handled) {
            std::cout << "Command not recognized. Type 'crowndb --help' for help.\n";
        }
    }
}
