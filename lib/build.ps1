$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$proj = Split-Path -Parent $root

Push-Location $proj

New-Item -ItemType Directory -Force -Path "lib\out" | Out-Null

$objects = @(
  "lib\out\CrownDB.o",
  "lib\out\EngineAPI.o",
  "lib\out\StorageEngine.o",
  "lib\out\Database.o",
  "lib\out\Table.o",
  "lib\out\Session.o",
  "lib\out\UserManager.o",
  "lib\out\SQLParser.o",
  "lib\out\Utils.o",
  "lib\out\Record.o"
)

g++ -std=c++17 -I include -I lib\include -c lib\src\CrownDB.cpp -o lib\out\CrownDB.o

g++ -std=c++17 -I include -I lib\include -c engine\EngineAPI.cpp -o lib\out\EngineAPI.o
g++ -std=c++17 -I include -I lib\include -c engine\StorageEngine.cpp -o lib\out\StorageEngine.o
g++ -std=c++17 -I include -I lib\include -c engine\Database.cpp -o lib\out\Database.o
g++ -std=c++17 -I include -I lib\include -c engine\Table.cpp -o lib\out\Table.o
g++ -std=c++17 -I include -I lib\include -c engine\Session.cpp -o lib\out\Session.o
g++ -std=c++17 -I include -I lib\include -c engine\UserManager.cpp -o lib\out\UserManager.o

g++ -std=c++17 -I include -I lib\include -c src\SQLParser.cpp -o lib\out\SQLParser.o
g++ -std=c++17 -I include -I lib\include -c src\Utils.cpp -o lib\out\Utils.o
g++ -std=c++17 -I include -I lib\include -c src\Record.cpp -o lib\out\Record.o

ar rcs lib\libcrowndb.a @objects

Pop-Location
