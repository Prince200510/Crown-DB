#ifndef STORAGE_ENGINE_H
#define STORAGE_ENGINE_H
#include <string>

class StorageEngine {
    public:
    void create_database_file(const std::string& path);
    void read_database_file(const std::string& path);\
    bool validate_database_file(const std::string& path);
    int read_table_count(const std::string& path);
    void update_table_count(const std::string& path, int newCount);
};

#endif