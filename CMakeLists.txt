cmake_minimum_required(VERSION 3.16)
project(CrownDB
    VERSION 1.0.0
    DESCRIPTION "A Three-Tier Relational Database Management System in C++17"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CORE_SOURCES
    src/SQLParser.cpp
    src/Record.cpp
    src/Utils.cpp
    engine/EngineAPI.cpp
    engine/StorageEngine.cpp
    engine/UserManager.cpp
    engine/Session.cpp
    engine/Database.cpp
    engine/Table.cpp
    engine/DatabaseEngine.cpp
    lib/src/CrownDB.cpp
)
set(CORE_HEADERS
    include/EngineAPI.h
    include/StorageEngine.h
    include/SQLParser.h
    include/Session.h
    include/SessionContext.h
    include/UserManager.h
    include/Database.h
    include/Table.h
    include/Record.h
    include/Utils.h
    include/DatabaseError.h
    include/Types.h
    include/ColumnMetadata.h
    include/QueryResult.h
    include/DatabaseEngine.h
    include/Expression.h
    lib/include/Crowndb.h
)


add_library(crowndb_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})

target_include_directories(crowndb_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/include
)

add_executable(crowndb_cli
    src/main.cpp
    src/Cli.cpp
    include/Cli.h
)

target_link_libraries(crowndb_cli PRIVATE crowndb_core)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/basic.cpp")
    add_executable(crowndb_example examples/basic.cpp)
    target_link_libraries(crowndb_example PRIVATE crowndb_core)
endif()
