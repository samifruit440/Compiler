## Example Demonstration

## Source Code
`examples/main.c`:
```scheme
return (add1 (sub1 (6 * 7 + 20 - 5)));
```

## Features Demonstrated
- Nested unary operations (`add1`, `sub1`)
- Mixed arithmetic operators (`+`, `-`, `*`)
- Operator precedence (`*` before `+` and `-`)
- Parenthesized expressions
- Complex expression evaluation

## Calculation Trace
1. `6 * 7 = 42`           (multiplication first, highest precedence)
2. `42 + 20 = 62`         (addition)
3. `62 - 5 = 57`          (subtraction)
4. `sub1(57) = 56`        (decrement by 1)
5. `add1(56) = 57`        (increment by 1)
6. `57 << 2 = 228`        (tag as fixnum)

**Final Result:** `228` (decimal) = `57` (untagged)

## Tokenization
18 tokens total:
- Keywords: `return`
- Operators: `*`, `+`, `-`, `)`
- Identifiers: `add1`, `sub1`
- Numbers: `6`, `7`, `20`, `5`
- Punctuation: parentheses, semicolon, EOF

## RTE vs CTE Comparison

### RTE (Runtime Evaluation)
**18 instructions**
- Computes `6*7`, adds `20`, subtracts `5` at runtime
- Calls `add1` and `sub1` as runtime operations
- Full computation visible in assembly

### CTE (Compile-Time Evaluation)
**4 instructions**
- Pre-computes entire expression during compilation
- Only moves the final result (`228`) into register
- **78% code reduction!**

### Assembly Example (CTE)
```asm
movl $228, %eax          # Move pre-computed result
movl %eax, %ebx          # Move to exit code register
movl $1, %eax            # Exit syscall number
int $0x80                # Call kernel
```

## Verification
- 19/19 tests passing in RTE mode
- 19/19 tests passing in CTE mode
- Both modes produce identical exit code (228)
