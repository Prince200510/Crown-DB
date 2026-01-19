#include "UserManager.h"
#include "Session.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstring>

static constexpr char USER_MAGIC[8] = {'C','R','O','W','N','U','S','R'};

static std::string current_user;

static std::filesystem::path resolve_users_path() {
    namespace fs = std::filesystem;
    const fs::path p1 = fs::path("data") / "users.sys";
    if (fs::exists(fs::path("data"))) {
        return p1;
    }

    const fs::path p2 = fs::path("..") / "data" / "users.sys";
    if (fs::exists(fs::path("..") / "data")) {
        return p2;
    }

    fs::create_directories(fs::path("data"));
    return p1;
}

static bool init_users_file(const std::filesystem::path& usersPath) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(usersPath.parent_path(), ec);
    std::ofstream out(usersPath, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out.write(USER_MAGIC, 8);
    int zero = 0;
    out.write(reinterpret_cast<const char*>(&zero), sizeof(int));
    return static_cast<bool>(out);
}

static uint64_t hash_password(const std::string& password) {
    uint64_t hash = 5381;
    for (char c : password) {
        hash = ((hash << 5) + hash) + static_cast<uint64_t>(c);
    }
    return hash;
}

bool UserManager::register_user(const std::string& username, const std::string& password) {
    const auto usersPath = resolve_users_path();
    std::fstream file(usersPath, std::ios::in | std::ios::out | std::ios::binary);

    if (!file) {
        if (!init_users_file(usersPath)) {
            std::cerr << "Failed to create users file.\n";
            return false;
        }
        file.open(usersPath, std::ios::in | std::ios::out | std::ios::binary);
    }
    if (!file) {
        std::cerr << "Failed to open users file.\n";
        return false;
    }

    char magic[8];
    int count = 0;
    file.read(magic, 8);
    file.read((char*)&count, sizeof(int));

    if (!file || std::memcmp(magic, USER_MAGIC, 8) != 0 || count < 0) {
        file.close();
        if (!init_users_file(usersPath)) {
            std::cerr << "Invalid or corrupted users file.\n";
            return false;
        }
        file.open(usersPath, std::ios::in | std::ios::out | std::ios::binary);
        if (!file) {
            std::cerr << "Invalid or corrupted users file.\n";
            return false;
        }
        file.read(magic, 8);
        file.read(reinterpret_cast<char*>(&count), sizeof(int));
        if (!file || std::memcmp(magic, USER_MAGIC, 8) != 0 || count != 0) {
            std::cerr << "Invalid or corrupted users file.\n";
            return false;
        }
    }

    if (username.empty()) {
        std::cerr << "Username cannot be empty.\n";
        return false;
    }

    for(int i = 0; i < count; i++){
        int length = 0;
        file.read((char*)&length, sizeof(int));
        if (!file || length <= 0) {
            std::cerr << "Invalid or corrupted users file.\n";
            return false;
        }
        std::string stored_username(length, '\0');
        file.read(&stored_username[0], length);
        uint64_t stored_hash = 0;
        file.read((char*)&stored_hash, sizeof(uint64_t));
        if (!file) {
            std::cerr << "Invalid or corrupted users file.\n";
            return false;
        }
        if (stored_username == username) {
            std::cerr << "Username already exists.\n";
            return false;
        }
    }

    uint64_t hash = hash_password(password);
    int ulength = static_cast<int>(username.size());

    file.clear();
    file.seekp(0, std::ios::end);

    file.write((char*)&ulength, sizeof(int));
    file.write(username.c_str(), ulength);
    file.write((char*)&hash, sizeof(uint64_t));
    
    count++;

    file.seekp(8);
    file.write((char*)&count, sizeof(int));
    file.close();

    std::cout << "User registered successfully.\n";
    return true;
}

bool UserManager::login_user(const std::string& username, const std::string& password){
    const auto usersPath = resolve_users_path();
    std::ifstream file(usersPath, std::ios::binary);

    if(!file){
        std::cerr << "No users registered.\n";
        return false;
    }

    char magic[8];
    int count = 0;
    file.read(magic, 8);
    file.read((char*)&count, sizeof(int));

    if (!file || std::memcmp(magic, USER_MAGIC, 8) != 0 || count < 0) {
        std::cerr << "Invalid or corrupted users file.\n";
        return false;
    }

    const uint64_t wanted_hash = hash_password(password);
    
    for(int i = 0; i < count; i++){
        int length = 0;
        file.read((char*)&length, sizeof(int));
        if (!file || length <= 0) {
            std::cerr << "Invalid or corrupted users file.\n";
            return false;
        }
        std::string stored_username(length, '\0');
        file.read(&stored_username[0], length);
        uint64_t stored_hash = 0;
        file.read((char*)&stored_hash, sizeof(uint64_t));
        if (!file) {
            std::cerr << "Invalid or corrupted users file.\n";
            return false;
        }

        if(stored_username == username){
            if(wanted_hash == stored_hash){
                current_user = username;
                Session::login(username);
                std::cout << "Login successful. Welcome, " << username << "!\n";
                return true;
            } else {
                std::cerr << "Incorrect password.\n";
                return false;
            }
        }
    }

    std::cerr << "User not found.\n";
    return false;
}

std::string UserManager::get_current_user() {
	return current_user;
}