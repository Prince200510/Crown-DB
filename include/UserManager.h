#pragma once
#include <string>

class UserManager {
public:
    static bool register_user(const std::string& username, const std::string& password);
    static bool login_user(const std::string& username, const std::string& password);
    static std::string get_current_user();
};