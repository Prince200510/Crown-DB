#ifndef CROWNDB_QUERY_RESULT_H
#define CROWNDB_QUERY_RESULT_H
#include "ColumnMetadata.h"
#include "DatabaseError.h"
#include "Types.h"
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

class QueryResult {
private:
    DatabaseError error_;
    std::vector<ColumnMetadata> columns_;
    std::vector<std::vector<TypedValue>> rows_;
    size_t rows_affected_;
    bool has_rows_;
    std::string raw_output_;
    bool has_raw_output_;

public:
    QueryResult() : error_(DatabaseError::ok()),
          rows_affected_(0),
          has_rows_(false),
          raw_output_(""),
          has_raw_output_(false) {}

    static QueryResult success(size_t rows_affected = 0) {
        QueryResult res;
        res.error_ = DatabaseError::ok();
        res.rows_affected_ = rows_affected;
        res.has_rows_ = false;
        res.has_raw_output_ = false;
        return res;
    }

    static QueryResult raw(std::string text) {
        QueryResult res;
        res.error_ = DatabaseError::ok();
        res.raw_output_ = std::move(text);
        res.has_raw_output_ = true;
        res.has_rows_ = false;
        return res;
    }

    static QueryResult failure(DatabaseError error) {
        QueryResult res;
        res.error_ = std::move(error);
        res.rows_affected_ = 0;
        res.has_rows_ = false;
        res.has_raw_output_ = false;
        return res;
    }

    static QueryResult failure(ErrorCode code, std::string message) {
        return failure(DatabaseError(code, std::move(message)));
    }

    static QueryResult table(std::vector<ColumnMetadata> columns, std::vector<std::vector<TypedValue>> rows) {
        QueryResult res;
        res.error_ = DatabaseError::ok();
        res.rows_affected_ = rows.size();
        res.has_rows_ = true;
        res.has_raw_output_ = false;
        res.columns_ = std::move(columns);
        res.rows_ = std::move(rows);
        return res;
    }

    bool is_ok() const { return error_.is_ok(); }
    const DatabaseError& error() const { return error_; }
    const std::vector<ColumnMetadata>& columns() const { return columns_; }
    const std::vector<std::vector<TypedValue>>& rows() const { return rows_; }
    size_t rows_affected() const { return rows_affected_; }
    bool has_rows() const { return has_rows_; }

    std::string to_string() const {
        if (!is_ok()) {
            return "Error: " + error_.message() + "\n";
        }

        if (has_raw_output_) {
            return raw_output_;
        }

        if (!has_rows_) {
            return "Query OK, " + std::to_string(rows_affected_) + " row(s) affected\n";
        }

        const size_t col_count = columns_.size();
        if (col_count == 0) {
            return "Empty set\n";
        }

        std::vector<size_t> widths(col_count, 0);
        for (size_t i = 0; i < col_count; ++i) {
            widths[i] = columns_[i].name().size();
        }

        std::vector<std::vector<std::string>> cell_strings;
        cell_strings.reserve(rows_.size());

        for (const auto& row : rows_) {
            std::vector<std::string> row_strs;
            row_strs.reserve(col_count);
            for (size_t i = 0; i < col_count; ++i) {
                std::string cell = (i < row.size()) ? row[i].to_string() : "NULL";
                if (cell.size() > widths[i]) {
                    widths[i] = cell.size();
                }
                row_strs.push_back(std::move(cell));
            }
            cell_strings.push_back(std::move(row_strs));
        }

        std::ostringstream out;

        auto print_border = [&]() {
            out << "+";
            for (size_t w : widths) {
                out << std::string(w + 2, '-') << "+";
            }
            out << "\n";
        };

        print_border();

        out << "|";
        for (size_t i = 0; i < col_count; ++i) {
            out << " " << std::left << std::setw(static_cast<int>(widths[i])) << columns_[i].name() << " |";
        }
        out << "\n";

        print_border();

        for (const auto& row_str : cell_strings) {
            out << "|";
            for (size_t i = 0; i < col_count; ++i) {
                out << " " << std::left << std::setw(static_cast<int>(widths[i])) << row_str[i] << " |";
            }
            out << "\n";
        }

        print_border();

        out << rows_.size() << " row" << (rows_.size() == 1 ? "" : "s") << " in set\n";
        return out.str();
    }
};

#endif
