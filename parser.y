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
  double fval;
  char* sval;
  minic::TypeKind type;
  minic::Expr* expr;
  minic::Stmt* stmt;
  minic::Block* block;
  minic::Program* program;
  std::vector<minic::Stmt*>* stmt_list;
}

%token INT FLOAT BOOL STRING
%token <sval> IDENT
%token <ival> INT_LIT
%token <fval> FLOAT_LIT
%token <sval> STRING_LIT

%token TRUE_LIT FALSE_LIT
%token IF ELSE WHILE PRINT

%token LBRACE RBRACE LPAREN RPAREN
%token SEMI ASSIGN

%left OR
%left AND
%left EQ NE
%left LT LE GT GE
%left PLUS MINUS
%left STAR SLASH
%right NOT UMINUS

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%type <program> program
%type <stmt_list> stmt_list
%type <stmt> stmt decl assign_stmt block_stmt if_stmt while_stmt print_stmt
%type <block> block
%type <expr> expr
%type <type> type_spec

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
  | if_stmt { $$ = $1; }
  | while_stmt { $$ = $1; }
  | print_stmt { $$ = $1; }
  ;

type_spec
  : INT { $$ = minic::TypeKind::Int; }
  | FLOAT { $$ = minic::TypeKind::Float; }
  | BOOL { $$ = minic::TypeKind::Bool; }
  | STRING { $$ = minic::TypeKind::String; }
  ;

decl
  : type_spec IDENT SEMI
    {
      auto* d = new minic::Decl($2, $1, nullptr);
      d->loc = loc();
      std::free($2);
      $$ = d;
    }
  | type_spec IDENT ASSIGN expr SEMI
    {
      auto* d = new minic::Decl($2, $1, std::unique_ptr<minic::Expr>($4));
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

if_stmt
  : IF LPAREN expr RPAREN stmt %prec LOWER_THAN_ELSE
    {
      auto* iff = new minic::If(std::unique_ptr<minic::Expr>($3), std::unique_ptr<minic::Stmt>($5), nullptr);
      iff->loc = loc();
      $$ = iff;
    }
  | IF LPAREN expr RPAREN stmt ELSE stmt
    {
      auto* iff = new minic::If(std::unique_ptr<minic::Expr>($3), std::unique_ptr<minic::Stmt>($5), std::unique_ptr<minic::Stmt>($7));
      iff->loc = loc();
      $$ = iff;
    }
  ;

while_stmt
  : WHILE LPAREN expr RPAREN stmt
    {
      auto* w = new minic::While(std::unique_ptr<minic::Expr>($3), std::unique_ptr<minic::Stmt>($5));
      w->loc = loc();
      $$ = w;
    }
  ;

print_stmt
  : PRINT expr SEMI
    {
      auto* p = new minic::Print(std::unique_ptr<minic::Expr>($2));
      p->loc = loc();
      $$ = p;
    }
  ;

expr
  : expr PLUS expr
    {
      auto* e = new minic::BinaryExpr(minic::BinOp::Add,
                                      std::unique_ptr<minic::Expr>($1),
                                      std::unique_ptr<minic::Expr>($3));
      e->loc = loc();
      $$ = e;
    }
  | expr MINUS expr
    {
      auto* e = new minic::BinaryExpr(minic::BinOp::Sub,
                                      std::unique_ptr<minic::Expr>($1),
                                      std::unique_ptr<minic::Expr>($3));
      e->loc = loc();
      $$ = e;
    }
  | expr STAR expr
    {
      auto* e = new minic::BinaryExpr(minic::BinOp::Mul,
                                      std::unique_ptr<minic::Expr>($1),
                                      std::unique_ptr<minic::Expr>($3));
      e->loc = loc();
      $$ = e;
    }
  | expr SLASH expr
    {
      auto* e = new minic::BinaryExpr(minic::BinOp::Div,
                                      std::unique_ptr<minic::Expr>($1),
                                      std::unique_ptr<minic::Expr>($3));
      e->loc = loc();
      $$ = e;
    }
  | expr EQ expr
    {
      auto* e = new minic::BinaryExpr(minic::BinOp::Eq,
                                      std::unique_ptr<minic::Expr>($1),
                                      std::unique_ptr<minic::Expr>($3));
      e->loc = loc();
      $$ = e;
    }
  | expr NE expr
    {
      auto* e = new minic::BinaryExpr(minic::BinOp::Ne,
                                      std::unique_ptr<minic::Expr>($1),
                                      std::unique_ptr<minic::Expr>($3));
      e->loc = loc();
      $$ = e;
    }
  | expr LT expr
    {
      auto* e = new minic::BinaryExpr(minic::BinOp::Lt,
                                      std::unique_ptr<minic::Expr>($1),
                                      std::unique_ptr<minic::Expr>($3));
      e->loc = loc();
      $$ = e;
    }
  | expr LE expr
    {
      auto* e = new minic::BinaryExpr(minic::BinOp::Le,
                                      std::unique_ptr<minic::Expr>($1),
                                      std::unique_ptr<minic::Expr>($3));
      e->loc = loc();
      $$ = e;
    }
  | expr GT expr
    {
      auto* e = new minic::BinaryExpr(minic::BinOp::Gt,
                                      std::unique_ptr<minic::Expr>($1),
                                      std::unique_ptr<minic::Expr>($3));
      e->loc = loc();
      $$ = e;
    }
  | expr GE expr
    {
      auto* e = new minic::BinaryExpr(minic::BinOp::Ge,
                                      std::unique_ptr<minic::Expr>($1),
                                      std::unique_ptr<minic::Expr>($3));
      e->loc = loc();
      $$ = e;
    }
  | expr AND expr
    {
      auto* e = new minic::BinaryExpr(minic::BinOp::And,
                                      std::unique_ptr<minic::Expr>($1),
                                      std::unique_ptr<minic::Expr>($3));
      e->loc = loc();
      $$ = e;
    }
  | expr OR expr
    {
      auto* e = new minic::BinaryExpr(minic::BinOp::Or,
                                      std::unique_ptr<minic::Expr>($1),
                                      std::unique_ptr<minic::Expr>($3));
      e->loc = loc();
      $$ = e;
    }
  | MINUS expr %prec UMINUS
    {
      auto* e = new minic::UnaryExpr(minic::UnaryOp::Neg,
                                     std::unique_ptr<minic::Expr>($2));
      e->loc = loc();
      $$ = e;
    }
  | NOT expr
    {
      auto* e = new minic::UnaryExpr(minic::UnaryOp::Not,
                                     std::unique_ptr<minic::Expr>($2));
      e->loc = loc();
      $$ = e;
    }
  | INT_LIT
    {
      auto* i = new minic::IntLiteral($1);
      i->loc = loc();
      $$ = i;
    }
  | FLOAT_LIT
    {
      auto* f = new minic::FloatLiteral($1);
      f->loc = loc();
      $$ = f;
    }
  | TRUE_LIT
    {
      auto* b = new minic::BoolLiteral(true);
      b->loc = loc();
      $$ = b;
    }
  | FALSE_LIT
    {
      auto* b = new minic::BoolLiteral(false);
      b->loc = loc();
      $$ = b;
    }
  | STRING_LIT
    {
      auto* s = new minic::StringLiteral($1);
      s->loc = loc();
      std::free($1);
      $$ = s;
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
