#ifndef SESSION_H
#define SESSION_H

#include <string>

class Session {
public:
    static bool logged_in;
    static std::string current_user;
    static std::string current_database;

    static void login(const std::string& user);
    static void logout();
    static bool is_logged_in();
};

#endif 