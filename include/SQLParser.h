
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
	SelectWhere
};

struct ColumnDef {
	std::string name;
	int type;
};

struct Statement {
	StatementType type = StatementType::Unknown;
	std::string databaseName;
	std::string tableName;
	std::string columnName;
	int columnType = 0;
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
