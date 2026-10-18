# Prompt: Build CrownDB documentation + intro website (MySQL-style)

You are an expert technical writer + front-end developer. Build a **documentation website** and an **introduction/landing website** for a small C++17 embedded/file-based database called **CrownDB**.

## Ground truth (do not invent features)

Product summary:
- CrownDB is a small **C++17 file-based database engine**.
- It includes an interactive CLI terminal: `crownDB.exe`.
- It includes a reusable C++ wrapper library (`CrownDB`) with a public header `Crowndb.h`.
- Data directories (relative paths):
  - Databases stored under `databases/<name>.db/`
  - Tables stored as binary `.tbl` files inside the database folder
  - Users stored in `data/users.sys`
- CLI is login-protected. Typical flow:
  - `REGISTER user pass;`
  - `LOGIN user pass;`
  - then SQL commands

Supported SQL subset (document exactly these):
- Databases: `CREATE DATABASE name;`, `DROP DATABASE name;`, `USE name;`
- Tables: `CREATE TABLE t (col TYPE, ...);`, `DROP TABLE t;`, `TRUNCATE TABLE t;`,
  `ALTER TABLE t ADD col TYPE;`, `ALTER TABLE t DROP col;`
- Inserts: `INSERT INTO t VALUES (...);`
- Selects:
  - `SELECT * FROM t;`
  - `SELECT col1, col2 FROM t;` (projection)
  - `SELECT * FROM t WHERE col = value;` (and other operators)
  - Aggregates: `COUNT(*)`, `SUM(col)`, `AVG(col)`, `MIN(col)`, `MAX(col)`
  - Grouped aggregate example: `SELECT name, MAX(age) FROM users;` (implicit group-by)

## Output requirements

Create TWO deliverables:

1) **Landing site** (intro website)
   - Goal: explain what CrownDB is, why it exists, who it’s for.
   - Content sections:
     - Hero: “CrownDB — a small C++17 file-based database”
     - Highlights (bullet list): CLI, embeddable library, file-based storage, SQL subset
     - Quickstart (copy-paste commands): build CLI with g++, run `crownDB.exe`, then a short SQL session.
     - “Use as a library” section: show a minimal C++ snippet with `CrownDB` API (`register_user`, `login`, `execute`, `query`, `last_error`).
     - Storage model: explain `databases/` and `data/users.sys` paths.
     - Clear disclaimers: not MySQL compatible, SQL subset only, binary table format.

2) **Documentation site**
   - A left-nav docs layout like classic database docs.
   - Pages required (do not add more pages):
     - Overview
     - Install / Build (Windows + MinGW g++)
     - CLI Guide
     - SQL Reference
     - C++ API Reference (wrapper)
     - Storage Format (high-level)
     - Troubleshooting

## UX / visual style

- Theme: **MySQL-like terminal vibe** (clean, developer-focused).
- Use a monospace “prompt” component for examples, similar to:
  - `crownDB> ` for CLI
  - `mysql> ` style formatting (but label it `crownDB> `)
- Keep it “good and decent”: readable typography, consistent spacing, no gimmicks.

## Technical constraints

- Generate content as a static website.
- Prefer a simple toolchain (choose one):
  - Option A: Markdown + a static site generator (e.g., Docusaurus / MkDocs / Astro)
  - Option B: Plain HTML/CSS with a tiny build step
- Must include:
  - `index` landing page
  - Docs pages as markdown
  - Code blocks for all examples

## Must include these exact examples

### Build CLI (Windows / MSYS2 MinGW g++)

```sh
g++ -std=c++17 -I include -I . \
  src/main.cpp src/Cli.cpp src/SQLParser.cpp src/Utils.cpp src/Record.cpp \
  engine/EngineAPI.cpp engine/Database.cpp engine/Table.cpp engine/StorageEngine.cpp engine/UserManager.cpp engine/Session.cpp \
  -o bin/crownDB.exe
```

### Example CLI session

```sql
REGISTER admin admin;
LOGIN admin admin;
CREATE DATABASE demo;
USE demo;
CREATE TABLE users (name VARCHAR(50), age INT);
INSERT INTO users VALUES ('alice', 30);
SELECT * FROM users;
```

### Example C++ usage (wrapper)

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

## Acceptance criteria

- The landing page reads like a real product site.
- The docs are structured, consistent, and accurate to the SQL subset above.
- No extra features are claimed.
- All code blocks are copy-pasteable.
