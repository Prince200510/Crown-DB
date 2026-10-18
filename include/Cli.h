#ifndef CLI_H
#define CLI_H
#include "DatabaseEngine.h"
#include "SessionContext.h"
#include<memory>

class CLI {
private:
    std::shared_ptr<DatabaseEngine> engine_;
    SessionContext session_;
public:
    CLI();
    explicit CLI(std::shared_ptr<DatabaseEngine> engine, SessionContext session);
    void start();  
};

#endif
