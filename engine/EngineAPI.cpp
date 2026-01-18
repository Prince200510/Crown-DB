#include "EngineAPI.h"
#include "StorageEngine.h"
#include <filesystem>

bool EngineAPI::create_database(const std::string& db_name) {
    std::filesystem::create_directories("databases");
    std::string db_dir = "databases/" + db_name + ".db";
    std::string meta_path = db_dir + "/db.meta";

    if (std::filesystem::exists(db_dir)) {
        return false;
    }

    StorageEngine storage_engine;
    std::filesystem::create_directories(db_dir);
    storage_engine.create_database_file(meta_path);
    return true;
}

bool EngineAPI::drop_database(const std::string& db_name) {
    std::string db_dir = "databases/" + db_name + ".db";
    
    if (!std::filesystem::exists(db_dir)) {
        return false;
    }

    std::filesystem::remove_all(db_dir);

    if(current_database_name == db_name) {
        current_database_name.clear();
    }

    return true;
}

bool EngineAPI::use_database(const std::string& db_name) {
    std::string db_dir = "databases/" + db_name + ".db";
    std::string meta_path = db_dir + "/db.meta";

    if (!std::filesystem::exists(meta_path)) {
        return false;
    }

    StorageEngine storage;
    if (!storage.validate_database_file(meta_path)) {
        return false;
    }

    current_database_name = db_name;
    return true;
}

bool EngineAPI::has_active_database() const {
    return !current_database_name.empty();
}

bool EngineAPI::create_table(const std::string& table_name, const std::vector<std::pair<std::string, int>>& columns) {
    if(current_database_name.empty()) {
        return false;
    }

    StorageEngine storage;
    const std::string meta_path = "databases/" + current_database_name + ".db/db.meta";
    if (!storage.validate_database_file(meta_path)) {
        return false;
    }
    return storage.createTable(meta_path, table_name, columns);
}

bool EngineAPI::insert_into(const std::string& table_name, const std::vector<std::string>& values) {
    if (current_database_name.empty()) {
        return false;
    }

    const std::string meta_path = "databases/" + current_database_name + ".db/db.meta";
    StorageEngine storage;
    if (!storage.validate_database_file(meta_path)) {
        return false;
    }

    const std::string table_path = "databases/" + current_database_name + ".db/" + table_name + ".tbl";
    return StorageEngine::insert_row(table_path, values);
}

bool EngineAPI::select_all(const std::string& table_name) {
    if (current_database_name.empty()) {
        return false;
    }

    const std::string meta_path = "databases/" + current_database_name + ".db/db.meta";
    StorageEngine storage;
    if (!storage.validate_database_file(meta_path)) {
        return false;
    }

    const std::string table_path = "databases/" + current_database_name + ".db/" + table_name + ".tbl";
    return StorageEngine::select_all(table_path);
}