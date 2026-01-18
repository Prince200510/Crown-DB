#ifndef STORAGE_ENGINE_H
#define STORAGE_ENGINE_H
#include <string>
#include <utility>
#include <vector>

class StorageEngine {
    public:
    void create_database_file(const std::string& path);
    void read_database_file(const std::string& path);
    bool validate_database_file(const std::string& path);
    int read_table_count(const std::string& path);
    void update_table_count(const std::string& path, int new_count);
    bool createTable(const std::string& db_path, const std::string& table_name, const std::vector<std::pair<std::string, int>>& columns);
    static bool insert_row(const std::string& table_path, const std::vector<std::string>& values);
    static bool select_all(const std::string& table_path);
};

#endif