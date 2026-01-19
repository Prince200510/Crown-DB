#include <Session.h>

bool Session::logged_in = false;
std::string Session::current_user = "";
std::string Session::current_database = "";

void Session::login(const std::string& user) {
    logged_in = true;
    current_user = user;
}

void Session::logout() {
    logged_in = false;
    current_user = "";
    current_database = "";
}

bool Session::is_logged_in() {
    return logged_in;
}