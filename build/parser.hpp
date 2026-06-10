/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     INT = 258,
     FLOAT = 259,
     BOOL = 260,
     STRING = 261,
     IDENT = 262,
     INT_LIT = 263,
     FLOAT_LIT = 264,
     STRING_LIT = 265,
     TRUE_LIT = 266,
     FALSE_LIT = 267,
     IF = 268,
     ELSE = 269,
     WHILE = 270,
     PRINT = 271,
     LBRACE = 272,
     RBRACE = 273,
     LPAREN = 274,
     RPAREN = 275,
     SEMI = 276,
     ASSIGN = 277,
     OR = 278,
     AND = 279,
     NE = 280,
     EQ = 281,
     GE = 282,
     GT = 283,
     LE = 284,
     LT = 285,
     MINUS = 286,
     PLUS = 287,
     SLASH = 288,
     STAR = 289,
     UMINUS = 290,
     NOT = 291,
     LOWER_THAN_ELSE = 292
   };
#endif
/* Tokens.  */
#define INT 258
#define FLOAT 259
#define BOOL 260
#define STRING 261
#define IDENT 262
#define INT_LIT 263
#define FLOAT_LIT 264
#define STRING_LIT 265
#define TRUE_LIT 266
#define FALSE_LIT 267
#define IF 268
#define ELSE 269
#define WHILE 270
#define PRINT 271
#define LBRACE 272
#define RBRACE 273
#define LPAREN 274
#define RPAREN 275
#define SEMI 276
#define ASSIGN 277
#define OR 278
#define AND 279
#define NE 280
#define EQ 281
#define GE 282
#define GT 283
#define LE 284
#define LT 285
#define MINUS 286
#define PLUS 287
#define SLASH 288
#define STAR 289
#define UMINUS 290
#define NOT 291
#define LOWER_THAN_ELSE 292




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 29 "parser.y"
{
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
/* Line 1529 of yacc.c.  */
#line 135 "build/parser.hpp"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

