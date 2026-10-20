#include "DatabaseEngine.h"
#include "EngineAPI.h"
#include "SQLParser.h"
#include "SessionContext.h"
#include "UserManager.h"
#include "Expression.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace {

TypedValue string_to_typed_value(const std::string& str) {
    if (str == "true" || str == "TRUE") return TypedValue(true);
    if (str == "false" || str == "FALSE") return TypedValue(false);
    if (str == "NULL" || str.empty()) return TypedValue::make_null();

    try {
        size_t idx = 0;
        long long ll = std::stoll(str, &idx);
        if (idx == str.size()) {
            return TypedValue(static_cast<int64_t>(ll));
        }
    } catch (...) {}

    try {
        size_t idx = 0;
        double d = std::stod(str, &idx);
        if (idx == str.size()) {
            return TypedValue(d);
        }
    } catch (...) {}

    return TypedValue(str);
}

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

        if (st.type == SQLParser::StatementType::SelectAll || 
            st.type == SQLParser::StatementType::SelectWhere || 
            st.type == SQLParser::StatementType::SelectColumns) {
            
            std::vector<std::string> col_names;
            std::vector<std::vector<std::string>> all_rows;
            if (!api_.get_table_rows(st.tableName.c_str(), col_names, all_rows)) {
                return QueryResult::failure(ErrorCode::TableNotFound, "Select failed: Table not found or invalid schema");
            }

            std::shared_ptr<Expression> active_expr = st.whereExpr;
            if (active_expr) {
                if (auto bin = std::dynamic_pointer_cast<BinaryExpression>(active_expr)) {
                    if (auto sub = std::dynamic_pointer_cast<SubqueryExpression>(bin->right())) {
                        QueryResult sub_res = execute(session, sub->sql());
                        if (sub_res.is_ok()) {
                            std::string raw = sub_res.to_string();
                            std::istringstream iss(raw);
                            std::string line;
                            std::string last_val;
                            while (iss >> line) {
                                if (!line.empty() && line != "NULL") last_val = line;
                            }
                            TypedValue sub_val = string_to_typed_value(last_val);
                            active_expr = std::make_shared<BinaryExpression>(bin->op(), bin->left(), std::make_shared<ConstantExpression>(sub_val));
                        }
                    }
                }
            }

            std::vector<std::vector<std::string>> matched_rows;
            for (const auto& row : all_rows) {
                bool matches = true;
                if (active_expr) {
                    std::vector<TypedValue> typed_row;
                    typed_row.reserve(row.size());
                    for (const auto& cell : row) {
                        typed_row.push_back(string_to_typed_value(cell));
                    }
                    TypedValue tv = active_expr->evaluate(col_names, typed_row);
                    if (tv.type() == DataType::Boolean) {
                        matches = tv.get_boolean();
                    } else if (tv.type() == DataType::Integer) {
                        matches = tv.get_integer() != 0;
                    } else {
                        matches = false;
                    }
                } else if (!st.whereColumn.empty()) {
                    auto it = std::find(col_names.begin(), col_names.end(), st.whereColumn);
                    if (it != col_names.end()) {
                        size_t idx = std::distance(col_names.begin(), it);
                        if (idx < row.size()) {
                            matches = evaluate_simple_condition(row[idx], st.whereOp, st.whereValue);
                        } else {
                            matches = false;
                        }
                    } else {
                        matches = false;
                    }
                }

                if (matches) {
                    matched_rows.push_back(row);
                }
            }

            if (st.type == SQLParser::StatementType::SelectColumns && !st.selectColumns.empty()) {
                std::vector<size_t> col_indices;
                for (const auto& sc : st.selectColumns) {
                    auto it = std::find(col_names.begin(), col_names.end(), sc);
                    if (it != col_names.end()) {
                        col_indices.push_back(std::distance(col_names.begin(), it));
                    }
                }
                std::vector<std::vector<std::string>> projected_rows;
                for (const auto& r : matched_rows) {
                    std::vector<std::string> proj;
                    for (size_t ci : col_indices) {
                        proj.push_back(ci < r.size() ? r[ci] : "");
                    }
                    projected_rows.push_back(proj);
                }
                return QueryResult::raw(format_table(st.selectColumns, projected_rows));
            }

            return QueryResult::raw(format_table(col_names, matched_rows));
        }

        std::ostringstream out;
        struct CoutGuard {
            std::ostream& os;
            std::streambuf* old;
            explicit CoutGuard(std::ostream& os, std::streambuf* next) : os(os), old(os.rdbuf(next)) {}
            ~CoutGuard() { os.rdbuf(old); }
        } guard(std::cout, out.rdbuf());

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

private:
    bool evaluate_simple_condition(const std::string& field_val, const std::string& op, const std::string& target_val) {
        try {
            double n1 = std::stod(field_val);
            double n2 = std::stod(target_val);
            if (op == "=" || op == "==") return n1 == n2;
            if (op == "!=") return n1 != n2;
            if (op == "<") return n1 < n2;
            if (op == "<=") return n1 <= n2;
            if (op == ">") return n1 > n2;
            if (op == ">=") return n1 >= n2;
        } catch (...) {
            if (op == "=" || op == "==") return field_val == target_val;
            if (op == "!=") return field_val != target_val;
            if (op == "<") return field_val < target_val;
            if (op == "<=") return field_val <= target_val;
            if (op == ">") return field_val > target_val;
            if (op == ">=") return field_val >= target_val;
        }
        return false;
    }

    std::string format_table(const std::vector<std::string>& headers, const std::vector<std::vector<std::string>>& rows) {
        if (headers.empty()) {
            return "Empty set\n";
        }

        std::vector<size_t> widths(headers.size(), 0);
        for (size_t i = 0; i < headers.size(); ++i) {
            widths[i] = std::max(widths[i], headers[i].size());
        }
        for (const auto& r : rows) {
            for (size_t i = 0; i < headers.size() && i < r.size(); ++i) {
                widths[i] = std::max(widths[i], r[i].size());
            }
        }

        std::ostringstream oss;
        auto print_sep = [&]() {
            oss << "+";
            for (size_t w : widths) {
                oss << std::string(w + 2, '-') << "+";
            }
            oss << "\n";
        };

        print_sep();
        oss << "|";
        for (size_t i = 0; i < headers.size(); ++i) {
            oss << " " << std::left << std::setw(static_cast<int>(widths[i])) << headers[i] << " |";
        }
        oss << "\n";
        print_sep();

        for (const auto& r : rows) {
            oss << "|";
            for (size_t i = 0; i < headers.size(); ++i) {
                std::string v = (i < r.size()) ? r[i] : "";
                oss << " " << std::left << std::setw(static_cast<int>(widths[i])) << v << " |";
            }
            oss << "\n";
        }
        print_sep();
        oss << rows.size() << " row(s) in set\n";
        return oss.str();
    }
};

}

std::unique_ptr<DatabaseEngine> DatabaseEngine::create() {
    return std::make_unique<DefaultDatabaseEngine>();
}
