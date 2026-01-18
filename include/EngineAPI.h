#ifndef ENGINE_API_H
#define ENGINE_API_H
#include <string>
#include <utility>
#include <vector>

class EngineAPI {
    private:
    std::string current_database_name;

    public:
    bool create_database(const std::string& db_name);
    bool drop_database(const std::string& name);
    bool use_database(const std::string& name);
    bool has_active_database() const;
    bool create_table(const std::string& table_name, const std::vector<std::pair<std::string, int>>& columns);
    bool insert_into(const std::string& table_name, const std::vector<std::string>& values);
    bool select_all(const std::string& table_name);
};

#endif