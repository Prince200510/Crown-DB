#ifndef CROWNDB_SESSION_CONTEXT_H
#define CROWNDB_SESSION_CONTEXT_H
#include <atomic>
#include <cstdint>
#include <string>

class SessionContext {
private:
    static inline std::atomic<uint64_t> next_id_{1};
    uint64_t session_id_;
    std::string user_;
    std::string current_database_;
    bool authenicated_;
    bool in_transaction_;

public:
    SessionContext() : session_id_(next_id_.fetch_add(1, std::memory_order_relaxed)),
    user_(""), current_database_(""), authenicated_(false), in_transaction_(false) {}

    explicit SessionContext(std::string username) : session_id_(next_id_.fetch_add(1, std::memory_order_relaxed)),
    user_(std::move(username)), current_database_(""), authenicated_(true), in_transaction_(false) {}

    uint64_t session_id() const { 
        return session_id_;
    }

    bool is_authenticated() const {
        return authenicated_;
    }

    const std::string& user() const {
        return user_;
    }

    void login(std::string username) {
        user_ = std::move(username);
        authenicated_ = true;
    }

    void logout() {
        user_.clear();
        current_database_.clear();
        authenicated_ = false;
        in_transaction_ = false;
    }

    bool has_active_database() const {
        return !current_database_.empty();
    }

    const std::string& current_database() const {
        return current_database_;
    }

    void set_current_database(std::string db_name) {
        current_database_ = std::move(db_name);
    }

    void clear_current_database() {
        current_database_.clear();
    }

    bool in_transaction() const {
        return in_transaction_;
    }

    void set_in_transaction(bool val) {
        in_transaction_ = val;
    }
};

#endif