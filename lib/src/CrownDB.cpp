#include "Crowndb.h"

#include "EngineAPI.h"
#include "SQLParser.h"
#include "Session.h"
#include "UserManager.h"

#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

struct CrownDB::Impl {
    EngineAPI engine;
    std::string current_database;
    std::string last_error;
};

static Aggregate_type toAggType(SQLParser::AggregateFunc f) {
    switch (f) {
        case SQLParser::AggregateFunc::Count: return Aggregate_type::COUNT;
        case SQLParser::AggregateFunc::Sum: return Aggregate_type::SUM;
        case SQLParser::AggregateFunc::Avg: return Aggregate_type::AVG;
        case SQLParser::AggregateFunc::Min: return Aggregate_type::MIN;
        case SQLParser::AggregateFunc::Max: return Aggregate_type::MAX;
    }
    return Aggregate_type::COUNT;
}

CrownDB::CrownDB(const std::string& db_name) : impl_(std::make_unique<Impl>()) {
    if (!db_name.empty()) {
        use_database(db_name);
    }
}

CrownDB::~CrownDB() = default;

CrownDB::CrownDB(CrownDB&&) noexcept = default;
CrownDB& CrownDB::operator=(CrownDB&&) noexcept = default;

bool CrownDB::use_database(const std::string& db_name) {
    impl_->last_error.clear();
    if (db_name.empty()) {
        impl_->last_error = "Database name cannot be empty";
        return false;
    }
    const bool ok = impl_->engine.use_database(db_name);
    if (!ok) {
        impl_->last_error = impl_->engine.get_last_error();
        return false;
    }
    impl_->current_database = db_name;
    return true;
}

bool CrownDB::register_user(const std::string& username, const std::string& password) {
    impl_->last_error.clear();
    if (!UserManager::register_user(username, password)) {
        impl_->last_error = "User registration failed";
        return false;
    }
    return true;
}

bool CrownDB::login(const std::string& username, const std::string& password) {
    impl_->last_error.clear();
    if (!UserManager::login_user(username, password)) {
        impl_->last_error = "Login failed";
        return false;
    }
    return true;
}

void CrownDB::logout() {
    Session::logout();
}

std::string CrownDB::current_user() const {
    return UserManager::get_current_user();
}

const std::string& CrownDB::last_error() const {
    return impl_->last_error;
}

static bool execute_statement(EngineAPI& engine, const SQLParser::Statement& st, std::string& last_error) {
    last_error.clear();

    switch (st.type) {
        case SQLParser::StatementType::CreateDatabase:
            if (!engine.create_database(st.databaseName)) {
                last_error = engine.get_last_error();
                return false;
            }
            return true;

        case SQLParser::StatementType::DropDatabase:
            if (!engine.drop_database(st.databaseName)) {
                last_error = engine.get_last_error();
                return false;
            }
            return true;

        case SQLParser::StatementType::UseDatabase:
            if (!engine.use_database(st.databaseName)) {
                last_error = engine.get_last_error();
                return false;
            }
            return true;

        case SQLParser::StatementType::CreateTable: {
            std::vector<std::pair<std::string, int>> cols;
            cols.reserve(st.columns.size());
            for (const auto& c : st.columns) {
                cols.emplace_back(c.name, c.type);
            }
            if (!engine.create_table(st.tableName, cols)) {
                last_error = engine.get_last_error().empty() ? "Failed to create table" : engine.get_last_error();
                return false;
            }
            return true;
        }

        case SQLParser::StatementType::DropTable:
            if (!engine.drop_table(st.tableName)) {
                last_error = engine.get_last_error().empty() ? "Failed to drop table" : engine.get_last_error();
                return false;
            }
            return true;

        case SQLParser::StatementType::TruncateTable:
            if (!engine.truncate_table(st.tableName)) {
                last_error = engine.get_last_error().empty() ? "Failed to truncate table" : engine.get_last_error();
                return false;
            }
            return true;

        case SQLParser::StatementType::AlterAddColumn: {
            const int baseType = (st.columnType & 0xFFFF);
            const int size = (st.columnType >> 16);
            if (!engine.alter_add_column(st.tableName, st.columnName, baseType, size)) {
                last_error = engine.get_last_error().empty() ? "Alter failed" : engine.get_last_error();
                return false;
            }
            return true;
        }

        case SQLParser::StatementType::AlterDropColumn:
            if (!engine.alter_drop_column(st.tableName, st.columnName)) {
                last_error = engine.get_last_error().empty() ? "Alter failed" : engine.get_last_error();
                return false;
            }
            return true;

        case SQLParser::StatementType::InsertInto:
            if (!engine.insert_into(st.tableName, st.values)) {
                last_error = engine.get_last_error().empty() ? "Insert failed" : engine.get_last_error();
                return false;
            }
            return true;

        case SQLParser::StatementType::SelectAll:
        case SQLParser::StatementType::SelectWhere:
        case SQLParser::StatementType::SelectColumns:
        case SQLParser::StatementType::SelectAggregate:
        case SQLParser::StatementType::SelectGroupAggregate:
            last_error = "Use query() for SELECT statements";
            return false;

        default:
            last_error = "Unsupported SQL statement";
            return false;
    }
}

bool CrownDB::execute(const std::string& sql_query) {
    impl_->last_error.clear();

    const auto parsed = SQLParser::parse(sql_query);
    if (!parsed.ok) {
        impl_->last_error = parsed.error;
        return false;
    }

    if (parsed.statement.type == SQLParser::StatementType::UseDatabase) {
        return use_database(parsed.statement.databaseName);
    }

    return execute_statement(impl_->engine, parsed.statement, impl_->last_error);
}

std::string CrownDB::query(const std::string& sql_query) {
    impl_->last_error.clear();

    const auto parsed = SQLParser::parse(sql_query);
    if (!parsed.ok) {
        impl_->last_error = parsed.error;
        return "";
    }

    std::ostringstream out;
    struct CoutGuard {
        std::ostream& os;
        std::streambuf* old;
        explicit CoutGuard(std::ostream& os, std::streambuf* next) : os(os), old(os.rdbuf(next)) {}
        ~CoutGuard() { os.rdbuf(old); }
    } guard(std::cout, out.rdbuf());

    const auto& st = parsed.statement;

    switch (st.type) {
        case SQLParser::StatementType::SelectAll:
            if (!impl_->engine.select_all(st.tableName)) {
                impl_->last_error = "Select failed";
                return "";
            }
            return out.str();

        case SQLParser::StatementType::SelectWhere: {
            Condition cond;
            cond.column_name = st.whereColumn;
            cond.op = st.whereOp;
            cond.value = st.whereValue;
            if (!impl_->engine.select_where(st.tableName, cond)) {
                impl_->last_error = "Select failed";
                return "";
            }
            return out.str();
        }

        case SQLParser::StatementType::SelectColumns: {
            const Condition* condPtr = nullptr;
            Condition cond;
            if (!st.whereColumn.empty()) {
                cond.column_name = st.whereColumn;
                cond.op = st.whereOp;
                cond.value = st.whereValue;
                condPtr = &cond;
            }
            if (!impl_->engine.select_columns(st.tableName, st.selectColumns, condPtr)) {
                impl_->last_error = "Select failed";
                return "";
            }
            return out.str();
        }

        case SQLParser::StatementType::SelectAggregate: {
            Aggregate_result agg;
            const bool ok = impl_->engine.aggregate(st.tableName, st.columnName, toAggType(st.aggregateFunc), agg);
            if (!ok) {
                impl_->last_error = impl_->engine.get_last_error();
                return "";
            }
            switch (st.aggregateFunc) {
                case SQLParser::AggregateFunc::Count: std::cout << agg.count << "\n"; break;
                case SQLParser::AggregateFunc::Sum: std::cout << agg.sum << "\n"; break;
                case SQLParser::AggregateFunc::Avg: std::cout << agg.avg << "\n"; break;
                case SQLParser::AggregateFunc::Min: std::cout << (agg.initialized ? std::to_string(agg.min) : "NULL") << "\n"; break;
                case SQLParser::AggregateFunc::Max: std::cout << (agg.initialized ? std::to_string(agg.max) : "NULL") << "\n"; break;
            }
            return out.str();
        }

        case SQLParser::StatementType::SelectGroupAggregate: {
            const Condition* condPtr = nullptr;
            Condition cond;
            if (!st.whereColumn.empty()) {
                cond.column_name = st.whereColumn;
                cond.op = st.whereOp;
                cond.value = st.whereValue;
                condPtr = &cond;
            }
            const bool ok = impl_->engine.group_aggregate(st.tableName, st.groupColumns, st.columnName, toAggType(st.aggregateFunc), condPtr);
            if (!ok) {
                impl_->last_error = impl_->engine.get_last_error();
                return "";
            }
            return out.str();
        }

        default:
            impl_->last_error = "Query supports only SELECT statements";
            return "";
    }
}
