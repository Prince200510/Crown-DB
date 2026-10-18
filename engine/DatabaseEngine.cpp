#ifndef CROWNDB_DATABASE_ENGINE_H
#define CROWNDB_DATABASE_ENGINE_H
#include "DatabaseError.h"
#include "QueryResult.h"
#include "SessionContext.h"
#include <memory>
#include <string>

class DatabaseEngine {
public:
    virtual ~DatabaseEngine() = default;
    virtual QueryResult execute(SessionContext& session, const std::string& sql) = 0;
    virtual bool register_user(const std::string& username, const std::string& password, DatabaseError& error) = 0;
    virtual bool authenticate_user(const std::string& username, const std::string& password, SessionContext& session, DatabaseError& error) = 0;

    static std::unique_ptr<DatabaseEngine> create();
};

#endif
