#ifndef STORAGE_ENGINE_H
#define STORAGE_ENGINE_H
#include <string>
#include <utility>
#include <vector>

struct Condition{
    std::string column_name;
    std::string op;
    std::string value;
};

class StorageEngine {
    public:
    void create_database_file(const std::string& path);
	void create_database_file(const std::string& path, const std::string& owner);
    void read_database_file(const std::string& path);
    bool validate_database_file(const std::string& path);
	bool read_database_owner(const std::string& path, const std::string& db_name, std::string& outOwner);
    int read_table_count(const std::string& path);
    void update_table_count(const std::string& path, int new_count);
    bool createTable(const std::string& db_path, const std::string& table_name, const std::vector<std::pair<std::string, int>>& columns);
    static bool insert_row(const std::string& table_path, const std::vector<std::string>& values);
    static bool select_all(const std::string& table_path);
    static bool drop_table(const std::string& table_path);
    static bool truncate_table(const std::string& table_path);
    static bool alter_add_column(const std::string& table_path, const std::string& column_name, int type, int size);
    static bool alter_drop_column(const std::string& table_path, const std::string& column_name);
    static bool select_where(const std::string& table_path, const Condition& cond);
};

#endif