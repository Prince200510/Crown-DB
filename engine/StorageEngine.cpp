#include "StorageEngine.h"
#include <fstream>
#include <cstring>
#include <filesystem>

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

bool StorageEngine::createTable(const std::string& db_path, const std::string& table_name, const std::vector<std::pair<std::string, int>>& columns) {
    namespace fs = std::filesystem;
    const fs::path metaFile(db_path);
    const fs::path dbDir = metaFile.parent_path();
    std::error_code ec;
    const fs::path tableFile = dbDir / (table_name + ".tbl");
    if (fs::exists(tableFile)) {
        return false;
    }

    {
        std::ofstream tbl(tableFile, std::ios::binary);
        if (!tbl) {
            return false;
        }

        char magic[4] = {'T', 'B', 'L', '1'};
        tbl.write(magic, sizeof(magic));

        int version = 1;
        tbl.write(reinterpret_cast<const char*>(&version), sizeof(version));

        int columns_count = static_cast<int>(columns.size());
        tbl.write(reinterpret_cast<const char*>(&columns_count), sizeof(columns_count));

        for (const auto& col : columns) {
            int col_name_length = static_cast<int>(col.first.size());
            tbl.write(reinterpret_cast<const char*>(&col_name_length), sizeof(col_name_length));
            tbl.write(col.first.c_str(), col_name_length);

            int type = col.second;
            tbl.write(reinterpret_cast<const char*>(&type), sizeof(type));
        }
    }

    std::fstream file(db_path, std::ios::in | std::ios::out | std::ios::binary);

    if (!file) {
        fs::remove(tableFile, ec);
        return false;
    }

    file.seekp(0, std::ios::end);

    int name_length = table_name.size();
    file.write(reinterpret_cast<const char*>(&name_length), sizeof(name_length));
    file.write(table_name.c_str(), name_length);

    int columns_count = columns.size();
    file.write(reinterpret_cast<const char*>(&columns_count), sizeof(columns_count));

    for(auto& col : columns) {
        int col_name_length = col.first.size();
        file.write(reinterpret_cast<const char*>(&col_name_length), sizeof(col_name_length));
        file.write(col.first.c_str(), col_name_length);
        
        int type = col.second;
        file.write(reinterpret_cast<char*>(&type), sizeof(type));    
    }

    file.close();

    int current_table_count = read_table_count(db_path);
    update_table_count(db_path, current_table_count + 1);

    return true;
}