# CrownDB

A small C++17 file-based database engine with:
- Interactive CLI (`bin/crownDB.exe`)
- A reusable C++ library wrapper (`CrownDB`) so other apps can use the engine without the CLI

It stores databases under `databases/` and user accounts under `data/users.sys`.

---

## Project layout

- `src/` — CLI + SQL parser + shared utilities
- `engine/` — core engine (storage, tables, DB metadata, session, users)
- `include/` — internal headers used by the CLI/engine
- `lib/` — public library wrapper (`lib/include/Crowndb.h`, `lib/src/CrownDB.cpp`)
- `examples/` — small “how to use the library” program
- `bin/` — built executables

---

## Build (Windows / MSYS2 MinGW g++)

### Build the CLI

From the project root (`P:\C++\crownDB`):

```sh
g++ -std=c++17 -I include -I . \
  src/main.cpp src/Cli.cpp src/SQLParser.cpp src/Utils.cpp src/Record.cpp \
  engine/EngineAPI.cpp engine/Database.cpp engine/Table.cpp engine/StorageEngine.cpp engine/UserManager.cpp engine/Session.cpp \
  -o bin/crownDB.exe
```

Run:

```sh
./bin/crownDB.exe
```

### Build the static library + example

You can build the library using the included script:

- PowerShell: run `lib/build.ps1`
- Bash/MSYS2: run `lib/build.sh`

Then build the example (from the project root):

```sh
g++ -std=c++17 -I lib/include -I include examples/basic.cpp lib/libcrowndb.a -o bin/crowndb_example.exe
./bin/crowndb_example.exe
```

More details: see [lib/BUILDING.md](lib/BUILDING.md)

---

## Using the CLI

Start the CLI:

```sh
./bin/crownDB.exe
```

### Authentication

The CLI is login-protected (you must login before running SQL).

Example flow:

```sql
REGISTER admin admin;
LOGIN admin admin;
CREATE DATABASE demo;
USE demo;
```

Notes:
- Users are stored in `data/users.sys`.
- Database ownership is enforced in `USE <db>`.

### Supported SQL (current)

The parser/engine supports a practical subset:

- Databases
  - `CREATE DATABASE name;`
  - `DROP DATABASE name;`
  - `USE name;`

- Tables
  - `CREATE TABLE t (col TYPE, ...);`
  - `DROP TABLE t;`
  - `TRUNCATE TABLE t;`
  - `ALTER TABLE t ADD col TYPE;`
  - `ALTER TABLE t DROP col;`

- Inserts
  - `INSERT INTO t VALUES (...);`

- Selects
  - `SELECT * FROM t;`
  - `SELECT * FROM t WHERE col = value;` (and other operators supported by `Condition`)
  - Projection: `SELECT col1, col2 FROM t;`
  - Aggregates:
    - `SELECT COUNT(*) FROM t;`
    - `SELECT SUM(col) FROM t;`
    - `SELECT AVG(col) FROM t;`
    - `SELECT MIN(col) FROM t;`
    - `SELECT MAX(col) FROM t;`
  - Grouped aggregate (implicit grouping):
    - `SELECT name, MAX(age) FROM users;`  
      This groups by `name` automatically.

---

## Using the library (no CLI)

Public header:
- [lib/include/Crowndb.h](lib/include/Crowndb.h)

Minimal example:

```cpp
#include "Crowndb.h"
#include <iostream>

int main() {
    CrownDB db;

    db.register_user("admin", "admin");
    if (!db.login("admin", "admin")) {
        std::cerr << db.last_error() << "\n";
        return 1;
    }

    if (!db.execute("CREATE DATABASE demo;")) {
        std::cerr << db.last_error() << "\n";
        return 1;
    }

    db.use_database("demo");
    db.execute("CREATE TABLE users (name VARCHAR(50), age INT);");
    db.execute("INSERT INTO users VALUES ('alice', 30);");

    std::cout << db.query("SELECT * FROM users;");
}
```

Key API behavior:
- `execute(sql)` is for non-SELECT statements and returns `bool`.
- `query(sql)` is for SELECT statements and returns a `std::string` containing the table output.
- `last_error()` returns the last error message produced by the wrapper.

---

## Storage notes

Tables are stored on disk as `.tbl` files (a custom **binary** table format — not plain text).
The `.tbl` file contains a magic header, schema metadata, and row data encoded in binary.

Databases are stored as directories under `databases/<name>.db/`, and each table is stored at:

`databases/<db>.db/<table>.tbl`

---

## Common issues

- **"Crowndb.h: No such file"**
  - You need the include path: compile with `-I ..\lib\include` (or `-I lib/include` from root).

- **Example runs but can’t find data/databases**
  - Run executables from the project root so relative paths like `data/` and `databases/` resolve.

- **"Access denied" when using a database**
  - The engine enforces ownership: login as the database owner first.

---

## License

Add your preferred license here.
