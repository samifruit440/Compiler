# Incremental Compiler with RTE/CTE Evaluation Modes

An x86-32 compiler implementing Ghuloum's incremental approach, featuring both **Runtime Evaluation (RTE)** and **Compile-Time Evaluation (CTE)** modes for comparison purposes.

Also kinda making a new language ??? 

## Features

- ✅ Integer literals with type tagging
- ✅ Binary operators: `+`, `-`, `*` with proper precedence
- ✅ Unary primitives: `add1`, `sub1`, `zero?`, predicates
- ✅ Immediate constants: booleans, characters, empty list
- ✅ **Local variables** with `let` expressions
- ✅ **Conditional expressions** with `if`
- ✅ **Heap allocation** with `cons`, `car`, `cdr`
- ✅ **Dual evaluation modes**: Runtime vs Compile-Time
- ✅ AST-based code generation with stack management

## Quick Start

```bash
make build-all    # Build both compilers
make test-all     # Run all tests and compare
```

## Language Syntax

```c
return <expr>;

// Immediate Constants
42                      // integer literal
#t, #f                  // booleans
#\A, #\space            // characters
()                      // empty list

// Arithmetic with Precedence (infix, top-level only)
2 + 3 * 4               // * binds tighter than +
10 - 5 + 3              // left-associative

// Prefix notation (inside parentheses only)
(+ 5 3)                 // addition as function call
(* 2 (+ 3 4))           // nested expressions

// Unary Primitives
(add1 5)                // returns 6
(sub1 5)                // returns 4
(zero? x)               // boolean predicate
(integer? x)            // type checking

// Binary Operators (prefix notation)
(+ 2 3)                 // addition
(- 10 4)                // subtraction
(* 3 4)                 // multiplication
(> x 5)                 // comparison: >, <, =, >=, <=

// Local Variables with let
(let (x 5) (+ x 3))    // Bind x to 5, evaluate (+ x 3)
(let (x 10) x)         // Returns 10

// Conditionals with if
(if #t 5 10)           // if true, returns 5; else 10
(if (> x 3) x 0)       // conditional based on expression

// Heap Allocation (pairs/lists)
(cons 5 10)            // allocate pair on heap
(car (cons 5 10))      // extract first element → 5
(cdr (cons 5 10))      // extract second element → 10
```

**Type Tagging:**
- Fixnums: `value << 2` (last 2 bits = `00`)
- Booleans: `#t` = 0x3F, `#f` = 0x1F
- Characters: `0x0F | (char << 8)`
- Empty list: `0x2F`

## Language Reference

### Immediate Constants

| Value | Syntax | Representation | Example |
|-------|--------|----------------|---------|
| Integer | `42` | `value << 2` (tag: 00) | `return 42;` |
| Boolean True | `#t` | `0x3F` | `return #t;` |
| Boolean False | `#f` | `0x1F` | `return #f;` |
| Character | `#\A` | `0x0F \| (char << 8)` | `return #\A;` |
| Empty List | `()` | `0x2F` | `return ();` |

### Arithmetic Operators

**Infix notation** (at top-level only, with operator precedence):

```scheme
2 + 3 - 1               → 4
10 * 2 + 5              → 25   (precedence: * before +)
2 + 3 * 4               → 14
```

**Prefix notation** (inside parentheses only):

```scheme
(+ 2 3)                 → 5
(- 10 4)                → 6
(* 3 4)                 → 12
(+ (* 2 3) 4)           → 10
(+ 2 (- 10 3))          → 9
```

**Important:** Infix and prefix are **mutually exclusive** per expression level:
- Top level uses infix: `2 + 3 * 4`
- Inside parens uses prefix: `(+ 2 (* 3 4))`
- Cannot mix: `(+ 2 3 * 4)` is **invalid**

**Operator Precedence** (infix only, highest to lowest):
1. Parenthesized expressions
2. Literals and single values
3. Multiplication: `*`
4. Addition/Subtraction: `+`, `-`

### Comparison & Logic

All comparison operators use **prefix notation**:

```scheme
(= 5 5)                 → #t   (equal)
(< 3 5)                 → #t   (less than)
(> 10 8)                → #t   (greater than)
(<= 5 5)                → #t   (less or equal)
(>= 5 5)                → #t   (greater or equal)

(char=? #\A #\B)       → #f   (character equality)
(char<? #\A #\B)       → #t   (character order)
```

### Type Predicates

Check the type of a value:

```scheme
(integer? 42)           → #t
(boolean? #t)           → #t
(char? #\A)             → #t
(null? ())              → #t
(zero? 0)               → #t
```

### Unary Operations

Transform or check values:

```scheme
(add1 5)                → 6
(sub1 5)                → 4
(integer->char 65)      → #\A    (int to char)
(char->integer #\A)     → 65     (char to int)
```

### Local Variables with `let`

Bind a value to a name within a scope:

```scheme
(let (x 5) x)                          → 5
(let (x 10) (+ x 5))                   → 15
(let (x 5) (if (> x 3) (* x 2) 0))     → 10
```

**Syntax:** `(let (var init-expr) body-expr)`
- **var**: Variable name to bind
- **init-expr**: Expression whose result is bound to var
- **body-expr**: Code executed with var in scope

**Scoping:** Variables are local to their let expression. Only the body can see the variable.

### Conditionals with `if`

Execute different code based on a condition:

```scheme
(if #t 5 10)                    → 5      (true branch)
(if #f 5 10)                    → 10     (false branch)
(if (> x 3) x 0)                → x or 0 (depends on test)
```

**Syntax:** `(if test consequent alternate)`
- **test**: Expression evaluated for truthiness
  - `#f` (0x1F) is the only false value
  - **Everything else is truthy** (including 0 as integer!)
- **consequent**: Executed if test is truthy
- **alternate**: Executed if test is false

### Heap Allocation with `cons`, `car`, `cdr`

Create and access pairs on the heap:

```scheme
(cons 5 10)             → pair with car=5, cdr=10
(car (cons 5 10))       → 5     (extract first)
(cdr (cons 5 10))       → 10    (extract second)

(cons 1 (cons 2 3))     → nested pairs
(car (cdr (cons 1 (cons 2 3))))  → 2
```

**Syntax:**
- `(cons car-expr cdr-expr)` - Allocate pair on heap
- `(car pair-expr)` - Extract first element
- `(cdr pair-expr)` - Extract second element

**Memory:** Pairs are 8-byte blocks on the heap:
- Bytes 0-3: first element (car)
- Bytes 4-7: second element (cdr)
- Pointer tag: 0x01 (distinguishes pairs from other types)

## Evaluation Modes

**See [detailed evaluation modes documentation](docs/EVALUATION_MODES.md)** for in-depth comparison with C++ constexpr knowledge.

### Runtime Evaluation (RTE) - Default
Generates assembly instructions that execute at runtime.

```bash
build/compiler-rte source.c
```

**Example:** `return 2 + 3 * 4;`
```asm
movl $16, %eax       # Load 4 (tagged)
movl %eax, -4(%esp)  # Save to stack
movl $12, %eax       # Load 3 (tagged)
movl -4(%esp), %ecx
imull %ecx, %eax     # Multiply
sarl $2, %eax        # Fix double-tagging
movl %eax, -4(%esp)
movl $8, %eax        # Load 2 (tagged)
addl -4(%esp), %eax  # Add
# Result: 56 (14 << 2)
```
*12 instructions*

### Compile-Time Evaluation (CTE) - Optimized
Pre-computes constant expressions during compilation.

```bash
build/compiler-cte -O source.c
```

**Example:** `return 2 + 3 * 4;`
```asm
movl $56, %eax       # Pre-computed: 14 << 2
```
*1 instruction !*

## Build System

| Command | Description |
|---------|-------------|
| `make build` | Build RTE compiler → `build/compiler-rte` |
| `make build-all` | Build both RTE and CTE compilers |
| `make test` | Run full test suite |
| `make test-all` | Test both modes + assembly comparison |
| `make clean` | Remove build artifacts |
| `make help` | Show all targets |

## Usage Examples

**See [detailed compilation guide](docs/COMPILE_AND_RUN.md)** for step-by-step explanation of each compilation stage.

### Compilation Pipeline Visibility

During compilation, the compiler automatically generates a **token stream file** (`out/tokens.txt`) showing you each step of tokenization. This is great for learning:

```bash
build/compiler-rte examples/main.c
cat out/tokens.txt
```

**Example output (`out/tokens.txt`):**
```
# Token Stream

Source: return (integer? 42);

## Tokens

Token 1: TOK_RETURN
Token 2: TOK_LPAREN
Token 3: TOK_IDENTIFIER (name: integer?)
Token 4: TOK_NUMBER (value: 42)
Token 5: TOK_RPAREN
Token 6: TOK_SEMICOLON
Token 7: TOK_EOF

## Summary

Total tokens: 7
```

This shows you exactly how the lexer breaks down your source code before passing tokens to the parser.

**Compile and run:**
```bash
# Using RTE mode
build/compiler-rte examples/main.c
as --32 out/output.s -o out/output.o
ld -m elf_i386 out/output.o -o out/program
./out/program
echo $?  # Shows tagged result

# Using CTE mode (optimized)
build/compiler-cte -O examples/main.c
```

**Compare modes:**
```bash
scripts/compare_modes.sh  # Side-by-side assembly comparison
```

## Understanding Test Output

Tests show tagged values as exit codes:

```
PASS: return 42; → Expected: 168 (42 as fixnum)
PASS: return #t; → Expected: 63 (as boolean)
```

**Why?** Type tagging encodes type information in the value:
- `42 << 2 = 168` (integer with `00` in last 2 bits)
- `#t = 0x3F = 63` (boolean with special bit pattern)
- `#\A = 0x0F | (65 << 8) = 16687` (character)

This allows runtime/GC to distinguish types by bit patterns.

## Project Structure

```
├── src/               # Compiler source (lexer, parser, AST, codegen)
├── test/              # Test suite
├── examples/          # Example programs
├── docs/              # Detailed documentation
├── scripts/           # Utility scripts (compare_modes.sh)
├── build/             # Compiled binaries
│   ├── compiler-rte   # Runtime evaluation compiler
│   └── compiler-cte   # Compile-time evaluation compiler
└── out/               # Generated assembly output
```

## When to Use Each Mode

**RTE (Runtime Evaluation):**
- Debugging and development
- Learning assembly generation
- When expressions contain variables (future)
- Full visibility into computation steps

**CTE (Compile-Time Evaluation):**
- Production builds
- Constant expression optimization
- Minimal code size
- Maximum performance

## Optimization Impact

| Expression | RTE Instructions | CTE Instructions | Reduction |
|------------|------------------|------------------|-----------|
| `2 + 3 * 4` | 12 | 4 | 67% |
| `(10 + 5) * 2 + 3` | 15 | 4 | 73% |
| `1 + 2 * 3` | 12 | 4 | 67% |

## Documentation

- **[README.md](README.md)** (this file) - Quick reference and overview
- **[docs/LANGUAGE_SYNTAX.md](docs/LANGUAGE_SYNTAX.md)** - Complete language syntax guide with examples
- **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** - Compiler design, data flow, and module breakdown
- **[docs/EVALUATION_MODES.md](docs/EVALUATION_MODES.md)** - RTE vs CTE comparison with C++ constexpr knowledge
- **[docs/COMPILE_AND_RUN.md](docs/COMPILE_AND_RUN.md)** - Step-by-step compilation and execution pipeline

## References

- [Ghuloum's "An Incremental Approach to Compiler Construction"](http://scheme2006.cs.uchicago.edu/11-ghuloum.pdf)
- x86-32 AT&T Assembly Syntax

## Next Steps

- [x] Local variables (let bindings)
- [x] Conditional expressions (if)
- [x] Heap allocation (cons/car/cdr)
- [ ] Multiple variable bindings: `(let ((x 5) (y 10)) ...)`
- [ ] Lambda expressions and closures
- [ ] String type and operations
- [ ] Vector/array type
- [ ] Mutable pairs: `set-car!`, `set-cdr!`
- [ ] Better error messages and error recovery
