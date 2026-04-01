%{
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ast.hpp"

// Flex interface
extern int yylex(void);
extern int yylineno;

extern int yycolumn;

void yyerror(const char* s);

minic::Program* g_program = nullptr;

static minic::SourceLoc loc() {
  minic::SourceLoc l;
  l.line = yylineno;
  l.col = yycolumn;
  return l;
}
%}

%union {
  int64_t ival;
  char* sval;
  minic::Expr* expr;
  minic::Stmt* stmt;
  minic::Block* block;
  minic::Program* program;
  std::vector<minic::Stmt*>* stmt_list;
}

%token INT
%token <sval> IDENT
%token <ival> INT_LIT

%token LBRACE RBRACE LPAREN RPAREN
%token SEMI ASSIGN
%token PLUS MINUS STAR SLASH

%type <program> program
%type <stmt_list> stmt_list
%type <stmt> stmt decl assign_stmt block_stmt
%type <block> block
%type <expr> expr term factor

%start program

%%

program
  : stmt_list
    {
      auto* p = new minic::Program();
      p->loc = loc();
      for (auto* s : *$1) p->stmts.emplace_back(s);
      delete $1;
      g_program = p;
      $$ = p;
    }
  ;

stmt_list
  : /* empty */
    {
      $$ = new std::vector<minic::Stmt*>();
    }
  | stmt_list stmt
    {
      $1->push_back($2);
      $$ = $1;
    }
  ;

stmt
  : decl { $$ = $1; }
  | assign_stmt { $$ = $1; }
  | block_stmt { $$ = $1; }
  ;

decl
  : INT IDENT SEMI
    {
      auto* d = new minic::Decl($2, minic::TypeKind::Int, nullptr);
      d->loc = loc();
      std::free($2);
      $$ = d;
    }
  | INT IDENT ASSIGN expr SEMI
    {
      auto* d = new minic::Decl($2, minic::TypeKind::Int, std::unique_ptr<minic::Expr>($4));
      d->loc = loc();
      std::free($2);
      $$ = d;
    }
  ;

assign_stmt
  : IDENT ASSIGN expr SEMI
    {
      auto* a = new minic::Assign($1, std::unique_ptr<minic::Expr>($3));
      a->loc = loc();
      std::free($1);
      $$ = a;
    }
  ;

block_stmt
  : block { $$ = $1; }
  ;

block
  : LBRACE stmt_list RBRACE
    {
      auto* b = new minic::Block();
      b->loc = loc();
      for (auto* s : *$2) b->stmts.emplace_back(s);
      delete $2;
      $$ = b;
    }
  ;

expr
  : expr PLUS term
    {
      auto* e = new minic::BinaryExpr(minic::BinOp::Add,
                                      std::unique_ptr<minic::Expr>($1),
                                      std::unique_ptr<minic::Expr>($3));
      e->loc = loc();
      $$ = e;
    }
  | expr MINUS term
    {
      auto* e = new minic::BinaryExpr(minic::BinOp::Sub,
                                      std::unique_ptr<minic::Expr>($1),
                                      std::unique_ptr<minic::Expr>($3));
      e->loc = loc();
      $$ = e;
    }
  | term { $$ = $1; }
  ;

term
  : term STAR factor
    {
      auto* e = new minic::BinaryExpr(minic::BinOp::Mul,
                                      std::unique_ptr<minic::Expr>($1),
                                      std::unique_ptr<minic::Expr>($3));
      e->loc = loc();
      $$ = e;
    }
  | term SLASH factor
    {
      auto* e = new minic::BinaryExpr(minic::BinOp::Div,
                                      std::unique_ptr<minic::Expr>($1),
                                      std::unique_ptr<minic::Expr>($3));
      e->loc = loc();
      $$ = e;
    }
  | factor { $$ = $1; }
  ;

factor
  : INT_LIT
    {
      auto* i = new minic::IntLiteral($1);
      i->loc = loc();
      $$ = i;
    }
  | IDENT
    {
      auto* v = new minic::VarRef($1);
      v->loc = loc();
      std::free($1);
      $$ = v;
    }
  | LPAREN expr RPAREN { $$ = $2; }
  ;

%%

void yyerror(const char* s) {
  std::fprintf(stderr, "line %d:%d: %s\n", yylineno, yycolumn, s);
}

