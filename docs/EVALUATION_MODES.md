# Evaluation Modes: RTE vs CTE

## Overview

The compiler supports two evaluation strategies for generating assembly code:

1. **Runtime Evaluation (RTE)** - Default mode
2. **Compile-Time Evaluation (CTE)** - Optimization mode (similar to C++'s `constexpr`)

**C++ Parallel**: This mirrors C++'s compile-time evaluation features:
- RTE is like regular runtime computation
- CTE is like `constexpr` evaluation that happens at compile-time

## Runtime Evaluation (RTE)

### Description
Generates assembly instructions for all operations. The computation happens when the program executes.

### Usage
```bash
build/compiler-rte source.c
```

### When to Use
- Development and debugging (easier to trace execution)
- When expressions contain variables (future feature)
- When you want to see the full generated code

### Example
Source: `return 2 + 3 * 4;`

Generated Assembly (12 instructions):
```asm
movl $16, %eax       # Load 4 (tagged: 4 << 2)
movl %eax, -4(%esp)  # Save to stack
movl $12, %eax       # Load 3 (tagged: 3 << 2)
movl -4(%esp), %ecx  # Load 4 from stack
imull %ecx, %eax     # Multiply: 3 * 4 = 12
sarl $2, %eax        # Fix double tagging
movl %eax, -4(%esp)  # Save result (48 = 12 << 2)
movl $8, %eax        # Load 2 (tagged: 2 << 2)
addl -4(%esp), %eax  # Add: 2 + 12 = 14
movl %eax, %ebx      # Move result to exit code
movl $1, %eax        # exit syscall
int $0x80
```

## Compile-Time Evaluation (CTE)

### Description
Pre-computes constant expressions during compilation. Only the final result is embedded in the assembly.

This is conceptually similar to C++'s `constexpr` keyword, which evaluates constant expressions at compile-time rather than runtime. In our compiler, when `-O` flag is used, all constant expressions are evaluated as if they were `constexpr` in C++.

### Usage
```bash
build/compiler-cte -O source.c
```

### When to Use
- Production builds (smaller, faster code)
- Expressions with only constants
- Maximum optimization

### Example
Source: `return 2 + 3 * 4;`

Generated Assembly (4 instructions):
```asm
movl $56, %eax       # Direct result: 14 << 2 = 56
movl %eax, %ebx      # Move to exit code
movl $1, %eax        # exit syscall
int $0x80
```

## Comparison

| Aspect | RTE | CTE | C++ Equivalent |
|--------|-----|-----|----------------|
| **Code Size** | Larger (full computation) | Smaller (pre-computed) | constexpr reduces code |
| **Performance** | Slower (runtime compute) | Faster (no computation) | constexpr at compile-time |
| **Optimization** | None | Constant folding | compiler optimizes |
| **Use Case** | Debugging, variables | Production, constants | constexpr for constants |
| **Instructions** | 12 for `2+3*4` | 4 for `2+3*4` | varies |
| **Syntax** | Default behavior | `-O` flag | `constexpr` keyword |

## Example: Complex Expression

Source: `return (10 + 5) * 2 + 3;`  
Mathematical result: `(10 + 5) * 2 + 3 = 15 * 2 + 3 = 33`

### RTE Output (15 instructions)
```asm
movl $12, %eax
movl %eax, -4(%esp)
movl $8, %eax
movl %eax, -8(%esp)
movl $20, %eax
movl %eax, -12(%esp)
movl $40, %eax
addl -12(%esp), %eax
movl -8(%esp), %ecx
imull %ecx, %eax
sarl $2, %eax
addl -4(%esp), %eax
movl %eax, %ebx
movl $1, %eax
int $0x80
```

### CTE Output (4 instructions)
```asm
movl $132, %eax      # 33 << 2 = 132
movl %eax, %ebx
movl $1, %eax
int $0x80
```

**Optimization:** CTE reduced code by 73% (11 instructions eliminated)

## Implementation Details

### How CTE Works

1. **Constant Detection**: `is_constant_expr()` recursively checks if all operands are literals
2. **Compile-Time Evaluation**: `eval_expr()` evaluates the entire expression tree at compile time
3. **Direct Emission**: Single `movl` instruction with the pre-computed result

### Supported Operations (CTE)

All operations with constant operands are optimized:
- Arithmetic: `+`, `-`, `*`
- Comparisons: `=`, `<`, `>`, `<=`, `>=`
- Unary: `add1`, `sub1`, `zero?`, `integer?`, `boolean?`, `char?`, `null?`
- Conversions: `integer->char`, `char->integer`

### Future: Mixed Expressions

When variables are added (future feature), CTE will perform **partial evaluation**:
- Constants folded at compile time
- Variables evaluated at runtime

Example: `return x + 2 * 3;` → CTE will optimize `2 * 3` to `6`, generating:
```asm
movl x_location, %eax
addl $24, %eax        # Add pre-computed 6 (as 24 = 6 << 2)
```

## C++ Knowledge: constexpr and Compile-Time Evaluation

### How CTE Relates to C++ constexpr

In C++, `constexpr` enables compile-time evaluation:

```cpp
// Evaluated at compile-time, result embedded in binary
constexpr int result = 2 + 3 * 4;  // Computed during compilation

// In our compiler (with -O flag):
// return 2 + 3 * 4;
// Produces same optimization: result directly in assembly
```

**Key Differences:**
- C++ `constexpr` requires explicit annotation
- Our compiler uses `-O` flag to enable CTE for all constant expressions
- Both eliminate runtime computation for constants
- Both embed the pre-computed result in the final binary

### Constant Folding

Both use **constant folding**, an optimization that:
1. Detects expressions with only constant operands
2. Evaluates the entire expression at compile-time
3. Replaces the entire expression tree with a single value

C++ Example:
```cpp
int arr[5 + 3];  // Array size computed at compile-time (8 elements)
```

Our Compiler:
```scheme
(return (+ 5 3))  ; With -O flag, becomes: movl $32, %eax (8 << 2)
```

### Future Extensions

When variables are added to our language, CTE will perform **partial evaluation** (like C++'s constexpr with non-constexpr parameters):
- Constants fold at compile-time
- Variables evaluated at runtime

Similar to C++'s mixed evaluation:
```cpp
constexpr int base = 10;
int compute(int x) {
    return x + base;  // x is variable, base is constexpr
}
```

## Verification

Both modes are tested to ensure they produce **identical results**. The test suite runs all 19 tests with both compilers and verifies exit codes match.

Run comparison:
```bash
make test-all
```

This will:
1. Run all tests with RTE
2. Run all tests with CTE
3. Compare assembly output and verify identical behavior
4. Show optimization statistics
