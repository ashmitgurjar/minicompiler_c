cmake_minimum_required(VERSION 3.20)
project(minicompiler LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

BISON_TARGET(Parser
  ${CMAKE_CURRENT_SOURCE_DIR}/parser.y
  ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
  DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.hpp
)

FLEX_TARGET(Scanner
  ${CMAKE_CURRENT_SOURCE_DIR}/lexer.l
  ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp
)

ADD_FLEX_BISON_DEPENDENCY(Scanner Parser)

add_executable(minic
  ${BISON_Parser_OUTPUTS}
  ${FLEX_Scanner_OUTPUTS}
  src/ast.cpp
  src/sema.cpp
  src/symbol_table.cpp
  src/main.cpp
)

target_include_directories(minic PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(minic PRIVATE
  YYERROR_VERBOSE=1
)

