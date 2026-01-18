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