## Mini-C (Flex + Bison + C++)

Mini-C is a tiny compiler front-end for a small subset of C. It includes:

- Flex scanner (`lexer.l`)
- Bison parser (`parser.y`)
- AST construction (C++)
- Symbol table + semantic analysis (scopes, declarations, use-before-declare, redeclare)

### Supported subset

- Types: `int`
- Declarations: `int x;`, `int x = expr;`
- Statements: assignment (`x = expr;`), block (`{ ... }`)
- Expressions: `+ - * /`, parentheses, integer literals, identifiers

### Build

Requires `flex`, `bison`, and a C++17 compiler.

```bash
cmake -S . -B build
cmake --build build
```

### Run

```bash
./build/minic examples/ok.mc
./build/minic examples/redeclare.mc
./build/minic examples/use_before_decl.mc
```

### Output

- On success: prints `OK` and exits 0
- On semantic error: prints a helpful error with line number and exits non-zero

