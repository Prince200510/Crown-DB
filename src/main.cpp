#include "Cli.h"
#include "UserManager.h"

#include <iostream>
#include <string>

int main() {
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
            UserManager::register_user(u, p);
            continue;
        }

        if (cmd == "login") {
            std::string u, p;
            std::cout << "Username: ";
            std::getline(std::cin, u);
            std::cout << "Password: ";
            std::getline(std::cin, p);

            if (UserManager::login_user(u, p)) {
                break;
            }
            continue;
        }
    }

    CLI cli;
    cli.start();
    return 0;
}
