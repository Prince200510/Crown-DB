## CrownDB: files to copy for your app (C++17)

If you want to use CrownDB **from your own C++ program** (without the CLI), you only need the public wrapper header + the built static library.

### Required (compile/link)

- `crown_db_output/include/Crowndb.h`
- `crown_db_output/lib/libcrowndb.a`

### Required (runtime working directory)

CrownDB uses **relative paths** for storage. When you run your program, your current working directory should contain:

- `data/users.sys` (user accounts)
- `databases/` (database folders)

This package includes a ready-to-run runtime layout here:

- `crown_db_output/runtime/data/users.sys`
- `crown_db_output/runtime/databases/`

Copy the `runtime/` folder contents next to your program EXE (or run from the project root).

### Minimal compile command (MinGW g++)

From your app folder (example):

```sh
g++ -std=c++17 -I ./crown_db_output/include your_app.cpp ./crown_db_output/lib/libcrowndb.a -o your_app.exe
```

### Example program

See: `crown_db_output/basic.cpp`
