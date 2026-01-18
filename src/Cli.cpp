#include "Cli.h"
#include "EngineAPI.h"
#include "SQLParser.h"

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

        const auto parsed = SQLParser::parse(rawStatement);
        if (!parsed.ok) {
            std::cout << parsed.error << "\n";
            continue;
        }

        const auto start = std::chrono::steady_clock::now();
        bool ok = false;
        switch (parsed.statement.type) {
            case SQLParser::StatementType::CreateDatabase:
                ok = engine_api.create_database(parsed.statement.databaseName);
                break;
            case SQLParser::StatementType::DropDatabase:
                ok = engine_api.drop_database(parsed.statement.databaseName);
                break;
            case SQLParser::StatementType::DropTable:
                if (!engine_api.has_active_database()) {
                    std::cout << "No database selected. Use: use <database_name>;\n";
                    continue;
                }
                ok = engine_api.drop_table(parsed.statement.tableName);
                break;
            case SQLParser::StatementType::TruncateTable:
                if (!engine_api.has_active_database()) {
                    std::cout << "No database selected. Use: use <database_name>;\n";
                    continue;
                }
                ok = engine_api.truncate_table(parsed.statement.tableName);
                break;
            case SQLParser::StatementType::AlterAddColumn: {
                if (!engine_api.has_active_database()) {
                    std::cout << "No database selected. Use: use <database_name>;\n";
                    continue;
                }
                const int baseType = (parsed.statement.columnType & 0xFFFF);
                const int size = (parsed.statement.columnType >> 16);
                ok = engine_api.alter_add_column(parsed.statement.tableName, parsed.statement.columnName, baseType, size);
                break;
            }
            case SQLParser::StatementType::AlterDropColumn:
                if (!engine_api.has_active_database()) {
                    std::cout << "No database selected. Use: use <database_name>;\n";
                    continue;
                }
                ok = engine_api.alter_drop_column(parsed.statement.tableName, parsed.statement.columnName);
                break;
            case SQLParser::StatementType::UseDatabase:
                ok = engine_api.use_database(parsed.statement.databaseName);
                break;
            case SQLParser::StatementType::CreateTable: {
                if (!engine_api.has_active_database()) {
                    std::cout << "No database selected. Use: use <database_name>;\n";
                    continue;
                }
                std::vector<std::pair<std::string, int>> cols;
                cols.reserve(parsed.statement.columns.size());
                for (const auto& c : parsed.statement.columns) {
                    cols.emplace_back(c.name, c.type);
                }
                ok = engine_api.create_table(parsed.statement.tableName, cols);
                break;
            }
            case SQLParser::StatementType::InsertInto:
                if (!engine_api.has_active_database()) {
                    std::cout << "No database selected. Use: use <database_name>;\n";
                    continue;
                }
                ok = engine_api.insert_into(parsed.statement.tableName, parsed.statement.values);
                break;
            case SQLParser::StatementType::SelectAll:
                if (!engine_api.has_active_database()) {
                    std::cout << "No database selected. Use: use <database_name>;\n";
                    continue;
                }
                ok = engine_api.select_all(parsed.statement.tableName);
                break;
            default:
                std::cout << "Unsupported SQL statement\n";
                continue;
        }
        const auto end = std::chrono::steady_clock::now();
        const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        switch (parsed.statement.type) {
            case SQLParser::StatementType::CreateDatabase:
                std::cout << (ok ? "Query OK, database created" : "Database already exists") << " (" << elapsedMs << " ms)\n";
                break;
            case SQLParser::StatementType::DropDatabase:
                std::cout << (ok ? "Query OK, database dropped" : "Database does not exist") << " (" << elapsedMs << " ms)\n";
                break;
            case SQLParser::StatementType::DropTable:
                std::cout << (ok ? "Query OK, table dropped" : "Table does not exist") << " (" << elapsedMs << " ms)\n";
                break;
            case SQLParser::StatementType::TruncateTable:
                std::cout << (ok ? "Query OK, table truncated" : "Truncate failed") << " (" << elapsedMs << " ms)\n";
                break;
            case SQLParser::StatementType::AlterAddColumn:
                std::cout << (ok ? "Query OK, column added" : "Alter failed") << " (" << elapsedMs << " ms)\n";
                break;
            case SQLParser::StatementType::AlterDropColumn:
                std::cout << (ok ? "Query OK, column dropped" : "Alter failed") << " (" << elapsedMs << " ms)\n";
                break;
            case SQLParser::StatementType::UseDatabase:
                std::cout << (ok ? "Database changed" : "Invalid or corrupted database") << " (" << elapsedMs << " ms)\n";
                break;
            case SQLParser::StatementType::CreateTable:
                std::cout << (ok ? "Query OK, table created" : "Failed to create table") << " (" << elapsedMs << " ms)\n";
                break;
            case SQLParser::StatementType::InsertInto:
                std::cout << (ok ? "Query OK, 1 row inserted" : "Insert failed") << " (" << elapsedMs << " ms)\n";
                break;
            case SQLParser::StatementType::SelectAll:
                if (!ok) {
                    std::cout << "Select failed" << " (" << elapsedMs << " ms)\n";
                }
                break;
            default:
                break;
        }
    }
}
