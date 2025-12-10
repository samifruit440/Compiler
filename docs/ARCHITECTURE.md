# Compiler Architecture

## Compilation Pipeline

```
Source File (.c)
      ↓
  [LEXER]        (src/lexer.c)
      ↓
   Tokens
      ↓
  [PARSER]       (src/parser.c)
      ↓
   AST Nodes    (src/ast.h)
      ↓
  [CODEGEN]      (src/codegen.c)
      ↓
 Assembly (.s)
      ↓
[as] Assembler
      ↓
 Object (.o)
      ↓
   [ld] Linker
      ↓
Executable
```

## Module Breakdown

### tags.h
- Defines type tagging constants and helpers
- **Exports:** Tagging masks/tags, `tag_fixnum()`, `untag_fixnum()`
- **No dependencies**

### ast.h / ast.c
- Abstract Syntax Tree node definitions and constructors
- **Implements:**
  - `ExprType` enum: FIXNUM, BOOLEAN, CHARACTER, EMPTY_LIST, UNARY_PRIM, BINARY_PRIM
  - `UnaryPrimType` enum: ADD1, SUB1, INTEGER_TO_CHAR, CHAR_TO_INTEGER, ZERO_P, NULL_P, INTEGER_P, BOOLEAN_P, CHAR_P
  - `BinaryPrimType` enum: PLUS, MINUS, MULTIPLY, EQUALS, LESS, GREATER, LESS_EQUAL, GREATER_EQUAL, CHAR_EQUAL, CHAR_LESS
  - Constructor functions for each expression type
  - Memory management: `expr_free()` for recursive cleanup
- **Exports:** All type definitions and constructors
- **Dependencies:** stdlib

### lexer.h / lexer.c
- Tokenization: converts source text → tokens
- **Recognizes:**
  - Keywords: `return`
  - Literals: numbers, `#t`, `#f`, `#\<char>`, `()`
  - Identifiers: function names with special characters (?, -, >)
  - Operators: `+`, `-`, `*`, `/`, `=`, `<`, `>`, `?`
  - Delimiters: `(`, `)`, `;`
- **Exports:** `TokenType`, `Token`, `lexer_init()`, `next_token()`
- **Dependencies:** stdio, stdlib, string, ctype

### parser.h / parser.c
- Parsing: converts tokens → AST nodes
- **Implements:**
  - Recursive descent parser with operator precedence
  - Primitive name mapping: string → UnaryPrimType/BinaryPrimType
  - Expression parsing with proper precedence:
    - `parse_expr()` - handles +/- (lowest precedence, left-associative)
    - `parse_term()` - handles * (higher precedence)
    - `parse_primary()` - handles literals, parenthesized expressions, and procedure calls
  - Procedure call recognition: `(primitive arg1 arg2 ...)`
  - Support for "return expr;" format (optional)
- **Returns:** AST `Expr*` for code generation
- **Exports:** `parse_program()`
- **Dependencies:** lexer.h, ast.h, stdio, stdlib, string

### codegen.h / codegen.c
- Code generation: converts AST → x86-32 AT&T assembly
- **Implements:**
  - Recursive code generation with stack management
  - Evaluation of constant expressions at compile time
  - Unary primitive code generation:
    - Arithmetic: add1 (addl $4), sub1 (subl $4)
    - Type checking: zero?, integer?, boolean?, char?, null?
    - Type conversion: integer->char, char->integer
  - Binary primitive code generation with proper operand ordering:
    - Arithmetic: +, -, * (with proper tagging adjustments)
    - Comparison: =, <, >, <=, >=, char=?, char<?
  - Stack index tracking for temporary value storage
- **Stack Model:**
  - Uses negative offsets from %esp (-4, -8, -12, ...)
  - SI (stack index) tracks current available offset
  - No actual %esp manipulation; pure offset-based access
  - Right operand saved first, left operand in %eax for operations
- **Exports:** `emit_program()`
- **Dependencies:** ast.h, tags.h, stdio, stdlib

### main.c
- Compiler driver: orchestrates lexer → parser → codegen pipeline
- **Implements:**
  - File I/O: reads source file
  - Error handling
  - Assembly output generation (to out/output.s)
  - Memory cleanup (expr and input)
- **Exports:** `main()`
- **Dependencies:** parser.h, codegen.h, ast.h, stdio, stdlib, sys/stat

### test/compiler_test.c
- Test harness
- **Process:**
  1. Create source file with test expression (format: "return expr;")
  2. Compile with `./compiler`
  3. Assemble with `as --32`
  4. Link with `ld -m elf_i386`
  5. Execute and check exit code
- **Coverage:** 19 tests
  - 4 integer literals
  - 4 addition tests
  - 2 subtraction tests
  - 2 multiplication tests
  - 2 grouping tests
  - 2 mixed expression tests
  - 2 boolean literal tests
  - 2 character literal tests
  - 1 empty list test

## Data Flow Example

Source: `return 2 + 3 * 4;`

1. **Lexer** → Tokens: `[RETURN, NUMBER(2), PLUS, NUMBER(3), STAR, NUMBER(4), SEMICOLON, EOF]`

2. **Parser** → AST:
   - Builds: `binary_prim(PLUS, fixnum(2), binary_prim(MULTIPLY, fixnum(3), fixnum(4)))`
   - Proper precedence: * has higher precedence than +
   - Returns AST for codegen

3. **Codegen** → Assembly (stack-based evaluation):
   - Evaluate right subtree first: `binary(MULTIPLY, 3, 4)`
     - Evaluate right: movl $16, %eax (4 << 2)
     - Save: movl %eax, -4(%esp)
     - Evaluate left: movl $12, %eax (3 << 2)
     - Multiply: imull %eax, %ecx; sarl $2, %eax → result 48 (12 << 2)
   - Back to parent: evaluate left operand
     - movl $8, %eax (2 << 2)
     - Add: addl -4(%esp), %eax → result 56

4. **Execution** → Exit code 56 (fixnum value 14 = 2 + (3*4) = 14)

## Key Design Decisions

1. **AST-Based Code Generation:** Parser builds AST, codegen recursively traverses it
   - Enables proper handling of nested expressions
   - Stack management naturally follows recursion depth
   - Separates concerns between parsing and code generation

2. **Stack-Based Binary Operations:**
   - Right operand evaluated first, saved to stack at offset SI
   - Left operand evaluated, result in %eax
   - Binary operation performed between %eax and stack location
   - Works for arbitrary nesting depth

3. **Type Tagging in AST:**
   - Immediate constants tagged when building AST nodes
   - Codegen evaluates tags at code generation time for immediates
   - Unary/binary primitives generate code that produces correct tags

4. **Modular Structure:**
   - Each phase has clear inputs/outputs
   - Easy to extend with new primitives
   - Simple data flow: Tokens → AST → Assembly

5. **No Register Allocation:**
   - Only uses %eax, %ecx, %esp
   - Simple and predictable for now
   - Can be extended to manage registers more efficiently

## Notable Implementation Details

### Multiplication Tagging
- Both operands are tagged (shifted left 2)
- Naive multiply: (a<<2) * (b<<2) = (a*b)<<4, which is doubly tagged
- Solution: `sarl $2` after multiply to compensate
- Correct: (a<<2) * (b<<2) >> 2 = (a*b) << 2

### Predicate Return Values
- All predicates return booleans (0x1F or 0x3F)
- Logic: sete/setl/etc puts boolean in %al, then:
  - movzbl %al, %eax → extend to full register
  - sall $6, %eax → shift to position
  - orl $0x3f, %eax → OR with boolean tag base

### Stack Index Tracking
- SI starts at -4 (first save location is -4(%esp))
- Decrements by 4 for each nested temporary
- Used with direct access (movl %d(%esp)) rather than pointer arithmetic
