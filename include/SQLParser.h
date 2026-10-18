#ifndef SQL_PARSER_H
#define SQL_PARSER_H

#include <string>
#include <utility>
#include <vector>

namespace SQLParser {

enum class StatementType {
	Unknown,
	CreateDatabase,
	DropDatabase,
	DropTable,
	TruncateTable,
	AlterAddColumn,
	AlterDropColumn,
	UseDatabase,
	CreateTable,
	InsertInto,
	SelectAll,
	SelectWhere,
	SelectAggregate,
	SelectColumns,
	SelectGroupAggregate
};

enum class AggregateFunc {
	Count,
	Sum,
	Avg,
	Min,
	Max
};

struct ColumnDef {
	std::string name;
	int type;
	bool is_primary_key = false;
	bool is_not_null = false;
	bool is_unique = false;
};

struct Statement {
	StatementType type = StatementType::Unknown;
	std::string databaseName;
	std::string tableName;
	std::string columnName;
	int columnType = 0;
	AggregateFunc aggregateFunc = AggregateFunc::Count;
	std::vector<std::string> selectColumns;
	std::vector<std::string> groupColumns;
	std::string whereColumn;
	std::string whereOp;
	std::string whereValue;
	std::vector<ColumnDef> columns;
	std::vector<std::string> values;
};

struct ParseResult {
	bool ok = false;
	Statement statement;
	std::string error;
};

ParseResult parse(const std::string& sql);

}

#endif
