#ifndef CROWNDB_DATABASE_ERROR_H
#define CROWNDB_DATABASE_ERROR_H
#include<string>

enum class ErrorCode {
    Ok,
    SyntaxError,
    DatabaseNotFound,
    DatabaseAlreadyExists,
    TableNotFound,
    TableAlreadyExists,
    ColumnNotFound,
    ColumnTypeMismatch,
    AccessDenied,
    UserNotFound,
    UserAlreadyExists,
    InvalidCredentials,
    NotLoggedIn,
    NoActiveDatabase,
    IOError,
    CorruptedData,
    InternalError
};

class DatabaseError {
private:
    ErrorCode code_;
    std::string message_;

public:
    DatabaseError() : code_(ErrorCode::Ok), message_("") {}
    DatabaseError(ErrorCode code, std::string message) : code_(code), message_(std::move(message)) {}
    ErrorCode code() const { return code_; }
    const std::string& message() const { return message_; }
    bool is_ok() const { return code_ == ErrorCode::Ok; }
    
    static DatabaseError ok() {
        return DatabaseError(ErrorCode::Ok, "");
    }
};

#endif