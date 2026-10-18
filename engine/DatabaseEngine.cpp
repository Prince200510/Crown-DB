#include "DatabaseEngine.h"
#include "EngineAPI.h"
#include "SQLParser.h"
#include "Session.h"
#include "UserManager.h"
#include <iostream>
#include <sstream>

namespace {

class DefaultDatabaseEngine : public DatabaseEngine {
private:
    EngineAPI api_;

    static Aggregate_type to_agg_type(SQLParser::AggregateFunc f) {
        switch (f) {
            case SQLParser::AggregateFunc::Count: return Aggregate_type::COUNT;
            case SQLParser::AggregateFunc::Sum: return Aggregate_type::SUM;
            case SQLParser::AggregateFunc::Avg: return Aggregate_type::AVG;
            case SQLParser::AggregateFunc::Min: return Aggregate_type::MIN;
            case SQLParser::AggregateFunc::Max: return Aggregate_type::MAX;
        }
        return Aggregate_type::COUNT;
    }

public:
    bool register_user(const std::string& username, const std::string& password, DatabaseError& error) override {
        if (username.empty()) {
            error = DatabaseError(ErrorCode::InvalidCredentials, "Username cannot be empty");
            return false;
        }
        if (!UserManager::register_user(username, password)) {
            error = DatabaseError(ErrorCode::UserAlreadyExists, "Registration failed or user already exists");
            return false;
        }
        error = DatabaseError::ok();
        return true;
    }

    bool authenticate_user(const std::string& username, const std::string& password, SessionContext& session, DatabaseError& error) override {
        if (!UserManager::login_user(username, password)) {
            error = DatabaseError(ErrorCode::InvalidCredentials, "Invalid username or password");
            return false;
        }
        session.login(username);
        error = DatabaseError::ok();
        return true;
    }

    QueryResult execute(SessionContext& session, const std::string& sql) override {
        if (!session.is_authenticated()) {
            return QueryResult::failure(ErrorCode::NotLoggedIn, "Please login first");
        }

        const auto parsed = SQLParser::parse(sql);
        if (!parsed.ok) {
            return QueryResult::failure(ErrorCode::SyntaxError, parsed.error);
        }

        const auto& st = parsed.statement;

        if (st.type == SQLParser::StatementType::CreateDatabase) {
            if (!api_.create_database(st.databaseName)) {
                return QueryResult::failure(ErrorCode::DatabaseAlreadyExists, api_.get_last_error());
            }
            return QueryResult::success(0);
        }

        if (st.type == SQLParser::StatementType::DropDatabase) {
            if (!api_.drop_database(st.databaseName)) {
                return QueryResult::failure(ErrorCode::DatabaseNotFound, api_.get_last_error());
            }
            if (session.current_database() == st.databaseName) {
                session.clear_current_database();
            }
            return QueryResult::success(0);
        }

        if (st.type == SQLParser::StatementType::UseDatabase) {
            if (!api_.use_database(st.databaseName)) {
                return QueryResult::failure(ErrorCode::AccessDenied, api_.get_last_error());
            }
            session.set_current_database(st.databaseName);
            return QueryResult::success(0);
        }

        if (!session.has_active_database()) {
            return QueryResult::failure(ErrorCode::NoActiveDatabase, "No database selected. Use 'USE <database>;' first");
        }

        if (api_.has_active_database() && session.current_database() != "") {
            api_.use_database(session.current_database());
        }

        if (st.type == SQLParser::StatementType::CreateTable) {
            std::vector<std::pair<std::string, int>> cols;
            cols.reserve(st.columns.size());
            for (const auto& c : st.columns) {
                cols.emplace_back(c.name, c.type);
            }
            if (!api_.create_table(st.tableName, cols)) {
                return QueryResult::failure(ErrorCode::TableAlreadyExists, api_.get_last_error());
            }
            return QueryResult::success(0);
        }

        if (st.type == SQLParser::StatementType::DropTable) {
            if (!api_.drop_table(st.tableName)) {
                return QueryResult::failure(ErrorCode::TableNotFound, api_.get_last_error());
            }
            return QueryResult::success(0);
        }

        if (st.type == SQLParser::StatementType::TruncateTable) {
            if (!api_.truncate_table(st.tableName)) {
                return QueryResult::failure(ErrorCode::TableNotFound, api_.get_last_error());
            }
            return QueryResult::success(0);
        }

        if (st.type == SQLParser::StatementType::AlterAddColumn) {
            const int baseType = (st.columnType & 0xFFFF);
            const int size = (st.columnType >> 16);
            if (!api_.alter_add_column(st.tableName, st.columnName, baseType, size)) {
                return QueryResult::failure(ErrorCode::InternalError, api_.get_last_error());
            }
            return QueryResult::success(0);
        }

        if (st.type == SQLParser::StatementType::AlterDropColumn) {
            if (!api_.alter_drop_column(st.tableName, st.columnName)) {
                return QueryResult::failure(ErrorCode::ColumnNotFound, api_.get_last_error());
            }
            return QueryResult::success(0);
        }

        if (st.type == SQLParser::StatementType::InsertInto) {
            if (!api_.insert_into(st.tableName, st.values)) {
                return QueryResult::failure(ErrorCode::InternalError, api_.get_last_error());
            }
            return QueryResult::success(1);
        }

                std::ostringstream out;
        struct CoutGuard {
            std::ostream& os;
            std::streambuf* old;
            explicit CoutGuard(std::ostream& os, std::streambuf* next) : os(os), old(os.rdbuf(next)) {}
            ~CoutGuard() { os.rdbuf(old); }
        } guard(std::cout, out.rdbuf());

        if (st.type == SQLParser::StatementType::SelectAll) {
            if (!api_.select_all(st.tableName)) {
                return QueryResult::failure(ErrorCode::TableNotFound, "Select failed: Table not found or invalid schema");
            }
            return QueryResult::raw(out.str());
        }

        if (st.type == SQLParser::StatementType::SelectWhere) {
            Condition cond;
            cond.column_name = st.whereColumn;
            cond.op = st.whereOp;
            cond.value = st.whereValue;
            if (!api_.select_where(st.tableName, cond)) {
                return QueryResult::failure(ErrorCode::TableNotFound, "Select failed");
            }
            return QueryResult::raw(out.str());
        }

        if (st.type == SQLParser::StatementType::SelectColumns) {
            const Condition* condPtr = nullptr;
            Condition cond;
            if (!st.whereColumn.empty()) {
                cond.column_name = st.whereColumn;
                cond.op = st.whereOp;
                cond.value = st.whereValue;
                condPtr = &cond;
            }
            if (!api_.select_columns(st.tableName, st.selectColumns, condPtr)) {
                return QueryResult::failure(ErrorCode::TableNotFound, "Select failed");
            }
            return QueryResult::raw(out.str());
        }

        if (st.type == SQLParser::StatementType::SelectAggregate) {
            Aggregate_result agg;
            if (!api_.aggregate(st.tableName, st.columnName, to_agg_type(st.aggregateFunc), agg)) {
                return QueryResult::failure(ErrorCode::TableNotFound, api_.get_last_error());
            }
            std::string agg_text;
            if (st.aggregateFunc == SQLParser::AggregateFunc::Count) {
                agg_text = std::to_string(agg.count) + "\n";
            } else if (st.aggregateFunc == SQLParser::AggregateFunc::Sum) {
                agg_text = std::to_string(agg.sum) + "\n";
            } else if (st.aggregateFunc == SQLParser::AggregateFunc::Avg) {
                agg_text = std::to_string(agg.avg) + "\n";
            } else if (st.aggregateFunc == SQLParser::AggregateFunc::Min) {
                agg_text = (agg.initialized ? std::to_string(agg.min) : "NULL") + "\n";
            } else {
                agg_text = (agg.initialized ? std::to_string(agg.max) : "NULL") + "\n";
            }
            return QueryResult::raw(std::move(agg_text));
        }

        if (st.type == SQLParser::StatementType::SelectGroupAggregate) {
            const Condition* condPtr = nullptr;
            Condition cond;
            if (!st.whereColumn.empty()) {
                cond.column_name = st.whereColumn;
                cond.op = st.whereOp;
                cond.value = st.whereValue;
                condPtr = &cond;
            }
            if (!api_.group_aggregate(st.tableName, st.groupColumns, st.columnName, to_agg_type(st.aggregateFunc), condPtr)) {
                return QueryResult::failure(ErrorCode::TableNotFound, "Group aggregate failed");
            }
            return QueryResult::raw(out.str());
        }

        return QueryResult::failure(ErrorCode::SyntaxError, "Unsupported SQL statement");
    }
};
}

std::unique_ptr<DatabaseEngine> DatabaseEngine::create() {
    return std::make_unique<DefaultDatabaseEngine>();
}
