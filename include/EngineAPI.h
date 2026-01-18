#ifndef ENGINE_API_H
#define ENGINE_API_H
#include <string>

class EngineAPI {
    private:
    std::string current_database_name;
    public:
    bool create_database(const std::string& db_name);
    bool drop_database(const std::string& name);
    bool use_database(const std::string& name);
    bool has_active_database() const;
};

#endif