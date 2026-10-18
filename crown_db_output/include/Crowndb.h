#pragma once

#include <memory>
#include <string>

class CrownDB {
public:
    explicit CrownDB(const std::string& db_name = "");
    ~CrownDB();

    CrownDB(const CrownDB&) = delete;
    CrownDB& operator=(const CrownDB&) = delete;

    CrownDB(CrownDB&&) noexcept;
    CrownDB& operator=(CrownDB&&) noexcept;

    bool register_user(const std::string& username, const std::string& password);
    bool login(const std::string& username, const std::string& password);
    void logout();
    std::string current_user() const;

    bool use_database(const std::string& db_name);

    bool execute(const std::string& sql_query);
    std::string query(const std::string& sql_query);

    const std::string& last_error() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
