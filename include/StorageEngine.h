#ifndef STORAGE_ENGINE_H
#define STORAGE_ENGINE_H
#include <string>

class StorageEngine {
    public:
    void create_database_file(const std::string& path);
    void read_database_file(const std::string& path);
};

#endif