#ifndef ENGINE_API_H
#define ENGINE_API_H
#include <string>
#include <utility>
#include <vector>

#include "StorageEngine.h"

class EngineAPI {
    private:
    std::string current_database_name;
	std::string last_error;

    public:
    bool create_database(const std::string& db_name);
    bool drop_database(const std::string& name);
    bool use_database(const std::string& name);
	const std::string& get_last_error() const { return last_error; }
    bool has_active_database() const;
    bool create_table(const std::string& table_name, const std::vector<std::pair<std::string, int>>& columns);
    bool insert_into(const std::string& table_name, const std::vector<std::string>& values);
    bool select_all(const std::string& table_name);
    bool drop_table(const std::string& table_name);
    bool truncate_table(const std::string& table_name);
    bool alter_add_column(const std::string& table_name, const std::string& column_name, int type, int size);
    bool alter_drop_column(const std::string& table_name, const std::string& column_name);
    bool select_where(const std::string& table_name, const Condition& cond);
	bool select_columns(const std::string& table_name, const std::vector<std::string>& columns, const Condition* cond = nullptr);
	bool group_aggregate(const std::string& table_name, const std::vector<std::string>& group_columns, const std::string& aggregate_column, Aggregate_type agg_type, const Condition* cond = nullptr);
	bool aggregate(const std::string& table_name, const std::string& column_name, Aggregate_type agg_type, Aggregate_result& out);
};

#endif