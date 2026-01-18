#include "EngineAPI.h"
#include "StorageEngine.h"
#include <filesystem>

bool EngineAPI::create_database(const std::string& db_name) {
    std::filesystem::create_directories("databases");
    std::string path = "databases/" + db_name + ".db";

    if (std::filesystem::exists(path)) {
        return false;
    }

    StorageEngine storage_engine;
    storage_engine.create_database_file(path);
    return true;
}

bool EngineAPI::drop_database(const std::string& db_name) {
    std::string path = "databases/" + db_name + ".db";
    
    if (!std::filesystem::exists(path)) {
        return false;
    }

    std::filesystem::remove(path);

    if(current_database_name == db_name) {
        current_database_name.clear();
    }

    return true;
}

bool EngineAPI::use_database(const std::string& db_name) {
    std::string path = "databases/" + db_name + ".db";

    if (!std::filesystem::exists(path)) {
        return false;
    }

    current_database_name = db_name;
    return true;
}

bool EngineAPI::has_active_database() const {
    return !current_database_name.empty();
}