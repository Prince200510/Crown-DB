#include "EngineAPI.h"
#include "StorageEngine.h"
#include "UserManager.h"
#include <filesystem>

bool EngineAPI::create_database(const std::string& db_name) {
	last_error.clear();
    std::filesystem::create_directories("databases");
    std::string db_dir = "databases/" + db_name + ".db";
    std::string meta_path = db_dir + "/db.meta";

    if (std::filesystem::exists(db_dir)) {
		last_error = "Database already exists";
        return false;
    }

    StorageEngine storage_engine;
    std::filesystem::create_directories(db_dir);
	const std::string owner = UserManager::get_current_user();
	storage_engine.create_database_file(meta_path, owner);
    return true;
}

bool EngineAPI::drop_database(const std::string& db_name) {
	last_error.clear();
    std::string db_dir = "databases/" + db_name + ".db";
    
    if (!std::filesystem::exists(db_dir)) {
		last_error = "Database does not exist";
        return false;
    }

    std::filesystem::remove_all(db_dir);

    if(current_database_name == db_name) {
        current_database_name.clear();
    }

    return true;
}

bool EngineAPI::use_database(const std::string& db_name) {
    last_error.clear();
    std::string db_dir = "databases/" + db_name + ".db";
    std::string meta_path = db_dir + "/db.meta";

    if (!std::filesystem::exists(meta_path)) {
        last_error = "Database does not exist";
        return false;
    }

    StorageEngine storage;
    if (!storage.validate_database_file(meta_path)) {
        last_error = "Invalid or corrupted database";
        return false;
    }

    std::string owner;
    if (!storage.read_database_owner(meta_path, db_name, owner)) {
        last_error = "Invalid or corrupted database";
        return false;
    }

    const std::string user = UserManager::get_current_user();
    if (!owner.empty() && owner != user) {
        last_error = "Access denied";
        return false;
    }
    if (owner.empty() && !user.empty() && owner != user) {
        last_error = "Access denied";
        return false;
    }

    current_database_name = db_name;
    return true;
}

bool EngineAPI::has_active_database() const {
    return !current_database_name.empty();
}

bool EngineAPI::create_table(const std::string& table_name, const std::vector<std::pair<std::string, int>>& columns) {
    last_error.clear();
    if(current_database_name.empty()) {
        last_error = "No database selected";
        return false;
    }

    StorageEngine storage;
    const std::string meta_path = "databases/" + current_database_name + ".db/db.meta";
    if (!storage.validate_database_file(meta_path)) {
        last_error = "Invalid or corrupted database";
        return false;
    }
    const bool ok = storage.createTable(meta_path, table_name, columns);
    if (!ok) {
        last_error = "Create table failed";
    }
    return ok;
}

bool EngineAPI::insert_into(const std::string& table_name, const std::vector<std::string>& values) {
    last_error.clear();
    if (current_database_name.empty()) {
        last_error = "No database selected";
        return false;
    }

    const std::string meta_path = "databases/" + current_database_name + ".db/db.meta";
    StorageEngine storage;
    if (!storage.validate_database_file(meta_path)) {
        last_error = "Invalid or corrupted database";
        return false;
    }

    const std::string table_path = "databases/" + current_database_name + ".db/" + table_name + ".tbl";
    const bool ok = StorageEngine::insert_row(table_path, values);
    if (!ok) {
        last_error = "Insert failed";
    }
    return ok;
}

bool EngineAPI::select_all(const std::string& table_name) {
    last_error.clear();
    if (current_database_name.empty()) {
        last_error = "No database selected";
        return false;
    }

    const std::string meta_path = "databases/" + current_database_name + ".db/db.meta";
    StorageEngine storage;
    if (!storage.validate_database_file(meta_path)) {
        last_error = "Invalid or corrupted database";
        return false;
    }

    const std::string table_path = "databases/" + current_database_name + ".db/" + table_name + ".tbl";
    const bool ok = StorageEngine::select_all(table_path);
    if (!ok) {
        last_error = "Select failed";
    }
    return ok;
}

bool EngineAPI::drop_table(const std::string& table_name) {
    last_error.clear();
    if (current_database_name.empty()) {
        last_error = "No database selected";
        return false;
    }

    const std::string meta_path = "databases/" + current_database_name + ".db/db.meta";
    StorageEngine storage;
    if (!storage.validate_database_file(meta_path)) {
        last_error = "Invalid or corrupted database";
        return false;
    }

    const std::string table_path = "databases/" + current_database_name + ".db/" + table_name + ".tbl";
    const bool ok = StorageEngine::drop_table(table_path);
    if (!ok) {
        last_error = "Drop table failed";
        return false;
    }

    const int count = storage.read_table_count(meta_path);
    if (count > 0) {
        storage.update_table_count(meta_path, count - 1);
    }
    return true;
}

bool EngineAPI::truncate_table(const std::string& table_name) {
    last_error.clear();
    if (current_database_name.empty()) {
        last_error = "No database selected";
        return false;
    }

    const std::string meta_path = "databases/" + current_database_name + ".db/db.meta";
    StorageEngine storage;
    if (!storage.validate_database_file(meta_path)) {
        last_error = "Invalid or corrupted database";
        return false;
    }

    const std::string table_path = "databases/" + current_database_name + ".db/" + table_name + ".tbl";
    const bool ok = StorageEngine::truncate_table(table_path);
    if (!ok) {
        last_error = "Truncate failed";
    }
    return ok;
}

bool EngineAPI::alter_add_column(const std::string& table_name, const std::string& column_name, int type, int size) {
    last_error.clear();
    if (current_database_name.empty()) {
        last_error = "No database selected";
        return false;
    }

    const std::string meta_path = "databases/" + current_database_name + ".db/db.meta";
    StorageEngine storage;
    if (!storage.validate_database_file(meta_path)) {
        last_error = "Invalid or corrupted database";
        return false;
    }

    const std::string table_path = "databases/" + current_database_name + ".db/" + table_name + ".tbl";
    const bool ok = StorageEngine::alter_add_column(table_path, column_name, type, size);
    if (!ok) {
        last_error = "Alter failed";
    }
    return ok;
}

bool EngineAPI::alter_drop_column(const std::string& table_name, const std::string& column_name) {
    last_error.clear();
    if (current_database_name.empty()) {
        last_error = "No database selected";
        return false;
    }

    const std::string meta_path = "databases/" + current_database_name + ".db/db.meta";
    StorageEngine storage;
    if (!storage.validate_database_file(meta_path)) {
        last_error = "Invalid or corrupted database";
        return false;
    }

    const std::string table_path = "databases/" + current_database_name + ".db/" + table_name + ".tbl";
    const bool ok = StorageEngine::alter_drop_column(table_path, column_name);
    if (!ok) {
        last_error = "Alter failed";
    }
    return ok;
}

bool EngineAPI::select_where(const std::string& table_name, const Condition& cond) {
    last_error.clear();
    if (current_database_name.empty()) {
        last_error = "No database selected";
        return false;
    }

    const std::string meta_path = "databases/" + current_database_name + ".db/db.meta";
    StorageEngine storage;
    if (!storage.validate_database_file(meta_path)) {
        last_error = "Invalid or corrupted database";
        return false;
    }

    const std::string table_path = "databases/" + current_database_name + ".db/" + table_name + ".tbl";
    const bool ok = StorageEngine::select_where(table_path, cond);
    if (!ok) {
        last_error = "Select failed";
    }
    return ok;
}

bool EngineAPI::select_columns(
    const std::string& table_name,
    const std::vector<std::string>& columns,
    const Condition* cond
) {
    last_error.clear();
    if (current_database_name.empty()) {
        last_error = "No database selected";
        return false;
    }
    const std::string table_path = "databases/" + current_database_name + ".db/" + table_name + ".tbl";
    const bool ok = StorageEngine::select_columns(table_path, columns, cond);
    if (!ok) {
        last_error = "Select failed";
    }
    return ok;
}

bool EngineAPI::group_aggregate(
    const std::string& table_name,
    const std::vector<std::string>& group_columns,
    const std::string& aggregate_column,
    Aggregate_type agg_type,
    const Condition* cond
) {
    last_error.clear();
    if (current_database_name.empty()) {
        last_error = "No database selected";
        return false;
    }
    const std::string table_path = "databases/" + current_database_name + ".db/" + table_name + ".tbl";
    const bool ok = StorageEngine::group_aggregate(table_path, group_columns, aggregate_column, agg_type, cond);
    if (!ok) {
        last_error = "Aggregation failed";
    }
    return ok;
}

bool EngineAPI::aggregate(
    const std::string& table_name,
    const std::string& column_name,
    Aggregate_type agg_type,
    Aggregate_result& out
) {
    last_error.clear();
    if (current_database_name.empty()) {
        last_error = "No database selected";
        return false;
    }

    const std::string table_path = "databases/" + current_database_name + ".db/" + table_name + ".tbl";
    if (!StorageEngine::aggregate_function(table_path, column_name, agg_type, out)) {
        last_error = "Aggregation failed";
        return false;
    }
    return true;
}