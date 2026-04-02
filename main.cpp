#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

#include "ast.hpp"
#include "print.hpp"
#include "sema.hpp"

// Token definitions from Bison (generated into build/parser.hpp).
#include "parser.hpp"

// Forward decls from Flex/Bison.
extern int yyparse(void);
extern int yylex(void);
extern FILE* yyin;

// Set by parser when parse succeeds.
extern minic::Program* g_program;

int main(int argc, char** argv) {
  if (argc < 2 || argc > 3) {
    std::cerr << "usage: minic [--tokens|--ast] <file>\n";
    return 2;
  }

  bool dumpTokens = false;
  bool dumpAst = false;
  const char* path = nullptr;

  if (argc == 2) {
    path = argv[1];
  } else {
    std::string opt = argv[1];
    if (opt == "--tokens") dumpTokens = true;
    else if (opt == "--ast") dumpAst = true;
    else {
      std::cerr << "unknown option: " << opt << "\n";
      return 2;
    }
    path = argv[2];
  }

  yyin = std::fopen(path, "rb");
  if (!yyin) {
    std::cerr << "error: cannot open file: " << path << "\n";
    return 2;
  }

  if (dumpTokens) {
    int tok = 0;
    while ((tok = yylex()) != 0) {
      switch (tok) {
        case INT: std::cout << "INT\n"; break;
        case IDENT: std::cout << "IDENT\n"; break;
        case INT_LIT: std::cout << "INT_LIT\n"; break;
        case LBRACE: std::cout << "LBRACE\n"; break;
        case RBRACE: std::cout << "RBRACE\n"; break;
        case LPAREN: std::cout << "LPAREN\n"; break;
        case RPAREN: std::cout << "RPAREN\n"; break;
        case SEMI: std::cout << "SEMI\n"; break;
        case ASSIGN: std::cout << "ASSIGN\n"; break;
        case PLUS: std::cout << "PLUS\n"; break;
        case MINUS: std::cout << "MINUS\n"; break;
        case STAR: std::cout << "STAR\n"; break;
        case SLASH: std::cout << "SLASH\n"; break;
        default:
          if (tok < 128 && std::isprint(tok)) std::cout << "CHAR(" << static_cast<char>(tok) << ")\n";
          else std::cout << "TOK(" << tok << ")\n";
          break;
      }
    }
    std::fclose(yyin);
    return 0;
  }

  int rc = yyparse();
  std::fclose(yyin);

  if (rc != 0 || g_program == nullptr) {
    std::cerr << "parse failed\n";
    return 1;
  }

  std::unique_ptr<minic::Program> program(g_program);
  g_program = nullptr;

  if (dumpAst) {
    minic::printAST(std::cout, *program);
    return 0;
  }

  try {
    minic::SemanticAnalyzer sema;
    sema.analyze(*program);
  } catch (const minic::SemanticError& e) {
    std::cerr << e.what() << "\n";
    return 1;
  } catch (const std::exception& e) {
    std::cerr << "internal error: " << e.what() << "\n";
    return 1;
  }

  std::cout << "OK\n";
  return 0;
}

