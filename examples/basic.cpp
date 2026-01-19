#include "Crowndb.h"

#include <iostream>

int main() {
    CrownDB db;

    db.register_user("admin", "admin");
    
    if (!db.login("admin", "admin")) {
        std::cout << "Login failed: " << db.last_error() << "\n";
        return 1;
    }

    if (db.execute("create database demo;")) {
        std::cout << "Database created successfully.\n";
    } else {
        std::cout << "Failed to create database: " << db.last_error() << "\n";
        return 1;
    }

    if (!db.use_database("demo")) {
        std::cout << "use_database failed: " << db.last_error() << "\n";
        return 1;
    }

    if (db.execute("create table users (id int, name VARCHAR(50), age INT);")) {
        std::cout << "Table created successfully.\n";
    } else {
        std::cout << "Failed to create table: " << db.last_error() << "\n";
        return 1;
    }
    
    const bool inserted =
        db.execute("insert into users values (1, 'Prince Maurya', 20);") &&
        db.execute("insert into users values (2, 'Suresh Maurya', 53);") &&
        db.execute("insert into users values (3, 'Sunidra Maurya', 51);");

    if (inserted) {
        std::cout << "Rows inserted successfully.\n";
    } else {
        std::cout << "Failed to insert rows: " << db.last_error() << "\n";
        return 1;
    }
    
    std::cout << "All rows:\n";
    std::cout << db.query("SELECT * FROM users;") << "\n";

    std::cout << "Projection:\n";
    std::cout << db.query("SELECT name, age FROM users;") << "\n";

    std::cout << "Grouped aggregate:\n";
    std::cout << db.query("SELECT name, MAX(age) FROM users;") << "\n";

    return 0;
}
