# Incremental Compiler with RTE/CTE Evaluation Modes

An x86-32 compiler implementing Ghuloum's incremental approach, featuring both **Runtime Evaluation (RTE)** and **Compile-Time Evaluation (CTE)** modes for comparison purposes.

## Features

- ✅ Integer literals with type tagging
- ✅ Binary operators: `+`, `-`, `*` with proper precedence
- ✅ Unary primitives: `add1`, `sub1`, `zero?`, predicates
- ✅ Immediate constants: booleans, characters, empty list
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

// Expressions
42                      // integer literal
#t, #f                  // booleans
#\A, #\space            // characters
()                      // empty list
2 + 3 * 4               // arithmetic with precedence
(add1 5)                // unary primitives
(zero? x)               // predicates
```

**Type Tagging:**
- Fixnums: `value << 2` (last 2 bits = `00`)
- Booleans: `#t` = 0x3F, `#f` = 0x1F
- Characters: `0x0F | (char << 8)`
- Empty list: `0x2F`

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
| `make test` | Run test suite (19 tests) |
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
- **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** - Compiler design, data flow, and module breakdown
- **[docs/EVALUATION_MODES.md](docs/EVALUATION_MODES.md)** - RTE vs CTE comparison with C++ constexpr knowledge
- **[docs/COMPILE_AND_RUN.md](docs/COMPILE_AND_RUN.md)** - Step-by-step compilation and execution pipeline

## References

- [Ghuloum's "An Incremental Approach to Compiler Construction"](http://scheme2006.cs.uchicago.edu/11-ghuloum.pdf)
- x86-32 AT&T Assembly Syntax

## Next Steps

- [ ] Local variables (let bindings)
- [ ] Conditional expressions (if)
- [ ] Heap allocation
- [ ] Lambda expressions and closures
- [ ] Proper error messages
