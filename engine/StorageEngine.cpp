#include "StorageEngine.h"
#include <fstream>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <iomanip>

using std::string;

void StorageEngine::create_database_file(const string& path) {
    std::ofstream file(path, std::ios::binary);

    if (!file) {
        throw std::runtime_error("Failed to create database file: " + path);
    }

    char magic[4] = {'C', 'D', 'B', '1'};
    file.write(magic, sizeof(magic));

    int version = 1;
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));

    int table_count = 0;
    file.write(reinterpret_cast<const char*>(&table_count), sizeof(table_count));

    file.close();
}

void StorageEngine::read_database_file(const string& path) {
    std::ifstream file(path, std::ios::binary);

    char magic[4];
    file.read(magic, sizeof(magic));

    int version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));

    int table_count;
    file.read(reinterpret_cast<char*>(&table_count), sizeof(table_count));
}

bool StorageEngine::validate_database_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    char magic[4];
    file.read(magic, 4);

    if (file.gcount() != 4) return false;

    if (std::strncmp(magic, "CDB1", 4) != 0) {
        return false;
    }

    int version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));

    if (version != 1) {
        return false;
    }

    return true;
}

int StorageEngine::read_table_count(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    
    if(!file) {
        return -1;
    }

    file.seekg(8);
    int count;

    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    return count;
}

void StorageEngine::update_table_count(const std::string& path, int new_count) {
    std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary);
    
    if(!file){
        return;
    }

    file.seekp(8);
    file.write(reinterpret_cast<const char*>(&new_count), sizeof(new_count)); 
    file.close();
}

bool StorageEngine::createTable(const std::string& db_path, const std::string& table_name, const std::vector<std::pair<std::string, int>>& columns) {
    namespace fs = std::filesystem;
    const fs::path metaFile(db_path);
    const fs::path dbDir = metaFile.parent_path();
    std::error_code ec;
    const fs::path tableFile = dbDir / (table_name + ".tbl");
    if (fs::exists(tableFile)) {
        return false;
    }

    {
        std::ofstream tbl(tableFile, std::ios::binary);
        if (!tbl) {
            return false;
        }

        const char magic[8] = {'C','R','O','W','N','T','B','L'};
        tbl.write(magic, sizeof(magic));

        const int columns_count = static_cast<int>(columns.size());
        int row_count = 0;
        tbl.write(reinterpret_cast<const char*>(&columns_count), sizeof(columns_count));
        tbl.write(reinterpret_cast<const char*>(&row_count), sizeof(row_count));

        for (const auto& col : columns) {
            const int col_name_length = static_cast<int>(col.first.size());
            tbl.write(reinterpret_cast<const char*>(&col_name_length), sizeof(col_name_length));
            tbl.write(col.first.c_str(), col_name_length);

            const int base_type = (col.second & 0xFFFF);
            const int size = (col.second >> 16);
            tbl.write(reinterpret_cast<const char*>(&base_type), sizeof(base_type));
            tbl.write(reinterpret_cast<const char*>(&size), sizeof(size));
        }
    }

    std::fstream file(db_path, std::ios::in | std::ios::out | std::ios::binary);

    if (!file) {
        fs::remove(tableFile, ec);
        return false;
    }

    file.seekp(0, std::ios::end);

    int name_length = table_name.size();
    file.write(reinterpret_cast<const char*>(&name_length), sizeof(name_length));
    file.write(table_name.c_str(), name_length);

    int columns_count = columns.size();
    file.write(reinterpret_cast<const char*>(&columns_count), sizeof(columns_count));

    for(auto& col : columns) {
        int col_name_length = col.first.size();
        file.write(reinterpret_cast<const char*>(&col_name_length), sizeof(col_name_length));
        file.write(col.first.c_str(), col_name_length);
        
        int type = col.second;
        file.write(reinterpret_cast<char*>(&type), sizeof(type));    
    }

    file.close();

    int current_table_count = read_table_count(db_path);
    update_table_count(db_path, current_table_count + 1);

    return true;
}

bool StorageEngine::insert_row(const std::string& table_path, const std::vector<std::string>& values) {
    auto ltrim = [](std::string& s) {
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
            s.erase(s.begin());
        }
    };
    auto rtrim = [](std::string& s) {
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
            s.pop_back();
        }
    };
    auto trimCopy = [&](std::string s) {
        ltrim(s);
        rtrim(s);
        return s;
    };

    auto parseIntStrict = [](const std::string& s, int& out) {
        size_t idx = 0;
        try {
            out = std::stoi(s, &idx);
        } catch (...) {
            return false;
        }
        return idx == s.size();
    };

    auto parseDateToInt = [&](const std::string& s, int& out) {
        std::string t = trimCopy(s);
        std::vector<int> parts;
        std::string cur;
        for (char ch : t) {
            if (ch == '-') {
                if (cur.empty()) return false;
                int v = 0;
                if (!parseIntStrict(cur, v)) return false;
                parts.push_back(v);
                cur.clear();
                continue;
            }
            if (!std::isdigit(static_cast<unsigned char>(ch))) return false;
            cur.push_back(ch);
        }
        if (cur.empty()) return false;
        int v = 0;
        if (!parseIntStrict(cur, v)) return false;
        parts.push_back(v);
        if (parts.size() != 3) return false;

        int day = 0, month = 0, year = 0;
        if (parts[0] >= 1000) {
            year = parts[0];
            month = parts[1];
            day = parts[2];
        } else if (parts[2] >= 1000) {
            day = parts[0];
            month = parts[1];
            year = parts[2];
        } else {
            return false;
        }
        if (year < 1 || month < 1 || month > 12 || day < 1 || day > 31) return false;
        out = year * 10000 + month * 100 + day;
        return true;
    };

    std::fstream file(table_path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file) {
        std::cerr << "Table not found\n";
        return false;
    }

    char magic[8];
    file.read(magic, 8);
    if (file.gcount() != 8 || std::string(magic, 8) != "CROWNTBL") {
        std::cerr << "Invalid table file\n";
        return false;
    }

    int column_count = 0;
    int row_count = 0;
    file.read(reinterpret_cast<char*>(&column_count), sizeof(int));
    file.read(reinterpret_cast<char*>(&row_count), sizeof(int));

    if (column_count <= 0) {
        std::cerr << "Invalid table schema\n";
        return false;
    }

    if (static_cast<int>(values.size()) != column_count) {
        std::cerr << "Column count mismatch\n";
        return false;
    }

    struct ColInfo { int type; int size; };
    std::vector<ColInfo> cols;
    cols.reserve(static_cast<size_t>(column_count));

    for (int i = 0; i < column_count; i++) {
        int name_length = 0;
        file.read(reinterpret_cast<char*>(&name_length), sizeof(int));
        if (name_length < 0) return false;
        file.seekg(name_length, std::ios::cur);

        int type = 0, size = 0;
        file.read(reinterpret_cast<char*>(&type), sizeof(int));
        file.read(reinterpret_cast<char*>(&size), sizeof(int));
        cols.push_back({type, size});
    }

    file.seekp(0, std::ios::end);

    constexpr int TYPE_INT = 1;
    constexpr int TYPE_VARCHAR = 2;
    constexpr int TYPE_DATE = 3;

    for (int i = 0; i < column_count; i++) {
        std::string v = trimCopy(values[static_cast<size_t>(i)]);
        const int type = cols[static_cast<size_t>(i)].type;
        const int size = cols[static_cast<size_t>(i)].size;

        if (type == TYPE_INT) {
            int int_value = 0;
            if (!parseIntStrict(v, int_value)) {
                std::cerr << "Invalid INT value: " << v << "\n";
                return false;
            }
            file.write(reinterpret_cast<const char*>(&int_value), sizeof(int_value));
        } else if (type == TYPE_DATE) {
            // strip optional quotes
            if (v.size() >= 2 && ((v.front() == '"' && v.back() == '"') || (v.front() == '\'' && v.back() == '\''))) {
                v = v.substr(1, v.size() - 2);
                v = trimCopy(v);
            }
            int date_value = 0;
            if (!parseDateToInt(v, date_value)) {
                std::cerr << "Invalid DATE value: " << v << "\n";
                return false;
            }
            file.write(reinterpret_cast<const char*>(&date_value), sizeof(date_value));
        } else if (type == TYPE_VARCHAR) {
            if (v.size() >= 2 && ((v.front() == '"' && v.back() == '"') || (v.front() == '\'' && v.back() == '\''))) {
                v = v.substr(1, v.size() - 2);
            }
            v = trimCopy(v);
            const int maxLen = (size > 0 ? size : 255);
            if (static_cast<int>(v.size()) > maxLen) {
                std::cerr << "VARCHAR too long (max " << maxLen << ")\n";
                return false;
            }
            const int str_length = static_cast<int>(v.size());
            file.write(reinterpret_cast<const char*>(&str_length), sizeof(str_length));
            file.write(v.c_str(), str_length);
        } else {
            std::cerr << "Unsupported column type\n";
            return false;
        }
    }

    row_count++;
    file.seekp(8 + static_cast<std::streamoff>(sizeof(int)), std::ios::beg);
    file.write(reinterpret_cast<const char*>(&row_count), sizeof(row_count));

    file.close();
    return true;
}

bool StorageEngine::select_all(const std::string& table_path) {
    std::ifstream file(table_path, std::ios::binary);
    if (!file) {
        std::cerr << "Table not found\n";
        return false;
    }

    char magic[8];
    file.read(magic, 8);
    if (file.gcount() != 8 || std::string(magic, 8) != "CROWNTBL") {
        std::cerr << "Invalid table file\n";
        return false;
    }

    int column_count = 0;
    int row_count = 0;
    file.read(reinterpret_cast<char*>(&column_count), sizeof(int));
    file.read(reinterpret_cast<char*>(&row_count), sizeof(int));
    if (column_count <= 0) {
        std::cerr << "Invalid table schema\n";
        return false;
    }

    struct ColInfo { std::string name; int type; int size; };
    std::vector<ColInfo> cols;
    cols.reserve(static_cast<size_t>(column_count));

    for (int i = 0; i < column_count; i++) {
        int name_length = 0;
        file.read(reinterpret_cast<char*>(&name_length), sizeof(int));
        if (name_length <= 0) return false;
        std::string name(static_cast<size_t>(name_length), '\0');
        file.read(&name[0], name_length);

        int type = 0, size = 0;
        file.read(reinterpret_cast<char*>(&type), sizeof(int));
        file.read(reinterpret_cast<char*>(&size), sizeof(int));
        cols.push_back({name, type, size});
    }

    constexpr int TYPE_INT = 1;
    constexpr int TYPE_VARCHAR = 2;
    constexpr int TYPE_DATE = 3;

    std::vector<std::vector<std::string>> rows;
    rows.reserve(static_cast<size_t>(row_count));
    std::vector<size_t> widths;
    widths.reserve(static_cast<size_t>(column_count));
    for (int i = 0; i < column_count; i++) {
        widths.push_back(cols[static_cast<size_t>(i)].name.size());
    }

    for (int r = 0; r < row_count; r++) {
        std::vector<std::string> row;
        row.reserve(static_cast<size_t>(column_count));
        for (int c = 0; c < column_count; c++) {
            const auto& col = cols[static_cast<size_t>(c)];
            std::string cell;
            if (col.type == TYPE_INT) {
                int v = 0;
                file.read(reinterpret_cast<char*>(&v), sizeof(int));
                cell = std::to_string(v);
            } else if (col.type == TYPE_DATE) {
                int d = 0;
                file.read(reinterpret_cast<char*>(&d), sizeof(int));
                const int year = d / 10000;
                const int month = (d / 100) % 100;
                const int day = d % 100;
                cell = std::to_string(year) + "-" + (month < 10 ? "0" : "") + std::to_string(month) + "-" + (day < 10 ? "0" : "") + std::to_string(day);
            } else if (col.type == TYPE_VARCHAR) {
                int len = 0;
                file.read(reinterpret_cast<char*>(&len), sizeof(int));
                if (len < 0) return false;
                std::string s(static_cast<size_t>(len), '\0');
                if (len > 0) {
                    file.read(&s[0], len);
                }
                cell = std::move(s);
            } else {
                std::cerr << "Unsupported column type\n";
                return false;
            }
            widths[static_cast<size_t>(c)] = std::max(widths[static_cast<size_t>(c)], cell.size());
            row.push_back(std::move(cell));
        }
        rows.push_back(std::move(row));
    }

    auto printBorder = [&](const std::vector<size_t>& w) {
        std::cout << "+";
        for (size_t i = 0; i < w.size(); i++) {
            std::cout << std::string(w[i] + 2, '-') << "+";
        }
        std::cout << "\n";
    };

    auto printRow = [&](const std::vector<std::string>& cells, const std::vector<size_t>& w) {
        std::cout << "|";
        for (size_t i = 0; i < w.size(); i++) {
            const std::string& v = (i < cells.size()) ? cells[i] : std::string();
            std::cout << " " << std::left << std::setw(static_cast<int>(w[i])) << v << " |";
        }
        std::cout << "\n";
    };

    printBorder(widths);
    {
        std::vector<std::string> header;
        header.reserve(cols.size());
        for (const auto& c : cols) header.push_back(c.name);
        printRow(header, widths);
    }
    printBorder(widths);
    for (const auto& r : rows) {
        printRow(r, widths);
    }
    printBorder(widths);
    std::cout << row_count << " row" << (row_count == 1 ? "" : "s") << " in set\n";
    return true;
}