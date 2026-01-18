#include "StorageEngine.h"
#include <fstream>
#include <cstring>

using std::string;

void StorageEngine::create_database_file(const string& path) {
    std::ofstream file(path, std::ios::binary);

    if (!file) {
        throw std::runtime_error("Failed to create database file: " + path);
    }

    char magic[4] = {'C', 'D', 'B', '1'};
    file.write(magic, sizeof(magic));

    int version = 1;
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));

    int table_count = 0;
    file.write(reinterpret_cast<const char*>(&table_count), sizeof(table_count));

    file.close();
}

void StorageEngine::read_database_file(const string& path) {
    std::ifstream file(path, std::ios::binary);

    char magic[4];
    file.read(magic, sizeof(magic));

    int version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));

    int table_count;
    file.read(reinterpret_cast<char*>(&table_count), sizeof(table_count));
}

bool StorageEngine::validate_database_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    char magic[4];
    file.read(magic, 4);

    if (file.gcount() != 4) return false;

    if (std::strncmp(magic, "CDB1", 4) != 0) {
        return false;
    }

    int version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));

    if (version != 1) {
        return false;
    }

    return true;
}

int StorageEngine::read_table_count(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    
    if(!file) {
        return -1;
    }

    file.seekg(8);
    int count;

    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    return count;
}

void StorageEngine::update_table_count(const std::string& path, int new_count) {
    std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary);
    
    if(!file){
        return;
    }

    file.seekp(8);
    file.write(reinterpret_cast<const char*>(&new_count), sizeof(new_count)); 
    file.close();
}