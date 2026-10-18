#include "Cli.h"
#include "DatabaseEngine.h"
#include "SessionContext.h"
#include <iostream>
#include <memory>
#include <string>

int main() {
    std::shared_ptr<DatabaseEngine> engine = DatabaseEngine::create();
    SessionContext session;

    while (true) {
        std::cout << "Type '1 to register' to create a new user or '2 tologin' to access an existing account:\n";
        std::cout << "crownDB client> ";
        std::string cmd;
        if (!std::getline(std::cin, cmd)) {
            return 0;
        }

        if (cmd == "1") cmd = "register";
        if (cmd == "2") cmd = "login";

        if (cmd == "register") {
            std::string u, p;
            std::cout << "Username: ";
            std::getline(std::cin, u);
            std::cout << "Password: ";
            std::getline(std::cin, p);

            DatabaseError err;
            if (engine->register_user(u, p, err)) {
                std::cout << "User registered successfully.\n";
            } else {
                std::cerr << err.message() << "\n";
            }
            continue;
        }

        if (cmd == "login") {
            std::string u, p;
            std::cout << "Username: ";
            std::getline(std::cin, u);
            std::cout << "Password: ";
            std::getline(std::cin, p);

            DatabaseError err;
            if (engine->authenticate_user(u, p, session, err)) {
                std::cout << "Login successful. Welcome, " << u << "!\n";
                break;
            } else {
                std::cerr << err.message() << "\n";
            }
            continue;
        }
    }

    CLI cli(engine, std::move(session));
    cli.start();
    return 0;
}
