# Language Syntax Guide

A comprehensive guide to writing programs in the Incremental Compiler language.

## Table of Contents

1. [Basic Syntax](#basic-syntax)
2. [Data Types](#data-types)
3. [Operators](#operators)
4. [Control Flow](#control-flow)
5. [Functions and Primitives](#functions-and-primitives)
6. [Variables and Scoping](#variables-and-scoping)
7. [Heap and Data Structures](#heap-and-data-structures)
8. [Examples](#examples)

## Basic Syntax

### Program Structure

Every program is a single expression that gets evaluated:

```c
return <expression>;
```

The `return` keyword and semicolon are optional (for backwards compatibility), but recommended.

### Comments

```c
// Single line comments start with //
return 42;  // This is a comment
```

Block comments are not currently supported.

## Data Types

All values in the language are represented with **type tags** embedded in their bit patterns.

### Integers (Fixnums)

Fixed-precision integers stored with a 2-bit tag:

```scheme
return 0;           // Zero
return 42;          // Positive integers
return -5;          // Negative integers (if supported)
```

**Representation:** `value << 2` (shifts value left 2 bits)
- Exit code: actual_value * 4
- Range: 30-bit signed integers (with 2-bit tag)

**Examples:**
```
42 → stored as 168 (exit code)
-1 → stored as -4 (exit code)
```

### Booleans

Two boolean values:

```scheme
return #t;          // True - represents success/yes
return #f;          // False - represents failure/no
```

**Representation:**
- `#t` = 0x3F (63 in decimal)
- `#f` = 0x1F (31 in decimal)

**Important:** In conditionals, **ONLY `#f` is false**. All other values are truthy, including the integer 0!

```scheme
(if 0 "yes" "no")       → "yes"    (0 is truthy as integer)
(if #f "yes" "no")      → "no"     (#f is falsy)
```

### Characters

Single character values:

```scheme
return #\A;         // Character 'A'
return #\space;     // Space character
return #\newline;   // Newline character
return #\x;         // Any single character
```

**Representation:** `0x0F | (char_code << 8)`
- Exit code: 15 + (char_code * 256)

**Examples:**
```
#\A (code 65)  → 15 + (65 * 256) = 16655
#\0 (code 48)  → 15 + (48 * 256) = 12303
```

### Empty List

The null/nil value:

```scheme
return ();          // Empty list
```

**Representation:** 0x2F (47 in decimal)

## Operators

### Arithmetic

#### Infix Notation (Top-Level Only)

Use natural infix operators at the top level of expressions:

```scheme
2 + 3                   → 5
10 - 4                  → 6
3 * 4                   → 12
2 + 3 * 4               → 14  (precedence: * before +)
10 * 2 + 5              → 25
```

#### Prefix Notation (Inside Parentheses Only)

Use parenthesized prefix notation for function-like calls:

```scheme
(+ 2 3)                 → 5
(- 10 4)                → 6
(* 3 4)                 → 12
(+ (* 2 3) 4)           → 10
(+ 2 (- 10 3))          → 9
```

#### Important: Infix vs Prefix

**Infix and prefix are mutually exclusive** per expression level:

```scheme
; Valid examples
(+ 2 3)                 ✓ Prefix
2 + 3                   ✓ Infix
(+ 2 (* 3 4))           ✓ Nested prefix
2 + (3 * 4)             ✓ Infix with grouping
2 * 3 + 4 * 5           ✓ Infix with precedence

; Invalid examples  
(+ 2 3 * 4)             ✗ Cannot use infix inside parentheses
2 + 3 (+ 4 5)           ✗ Cannot mix at same level
```

#### Addition and Subtraction

Infix form (top-level only):
```scheme
5 + 3                   → 8
10 - 4                  → 6
2 + 3 * 4               → 14  (* has higher precedence)
```

Prefix form (inside parentheses only):
```scheme
(+ 5 3)                 → 8
(- 10 4)                → 6
(+ 2 (* 3 4))           → 14
```

#### Multiplication

Infix form (top-level only):
```scheme
3 * 4                   → 12
2 * 3 + 4               → 10  (+ has lower precedence)
(2 + 3) * 4             → 20  (parentheses for grouping)
```

Prefix form (inside parentheses only):
```scheme
(* 3 4)                 → 12
(+ (* 2 3) 4)           → 10
(* (+ 2 3) 4)           → 20
```

#### Special Arithmetic

```scheme
(add1 x)                        → x + 1
(sub1 x)                        → x - 1

(add1 5)                        → 6
(sub1 10)                       → 9
```

### Comparisons

All comparison operators use prefix (functional) notation:

```scheme
(= 5 5)                         → #t
(= 5 3)                         → #f

(< 3 5)                         → #t
(< 5 3)                         → #f

(> 5 3)                         → #t
(> 3 5)                         → #f

(<= 5 5)                        → #t
(<= 5 3)                        → #f

(>= 5 5)                        → #t
(>= 3 5)                        → #f

; Character comparisons
(char=? #\A #\A)               → #t
(char=? #\A #\B)               → #f

(char<? #\A #\B)               → #t
(char<? #\B #\A)               → #f
```

All comparisons return `#t` or `#f`.

### Type Checking

Determine the type of a value:

```scheme
(integer? 42)                   → #t
(integer? #t)                   → #f
(integer? #\A)                  → #f
(integer? ())                   → #f

(boolean? #t)                   → #t
(boolean? #f)                   → #t
(boolean? 42)                   → #f

(char? #\A)                     → #t
(char? 42)                      → #f

(null? ())                      → #t
(null? 42)                      → #f

(zero? 0)                       → #t
(zero? 1)                       → #f
(zero? #t)                      → #f
```

### Type Conversions

Convert between types:

```scheme
(integer->char 65)              → #\A
(integer->char 97)              → #\a
(integer->char 32)              → #\space

(char->integer #\A)             → 65
(char->integer #\a)             → 97
(char->integer #\space)         → 32
```

## Control Flow

### Conditional Expressions with `if`

Execute different branches based on a condition:

```scheme
(if test consequent alternate)
```

**Semantics:**
1. Evaluate `test`
2. If test is NOT `#f`, evaluate and return `consequent`
3. If test IS `#f`, evaluate and return `alternate`

**Examples:**

```scheme
(if #t 5 10)                    → 5

(if #f 5 10)                    → 10

(if (> 10 5) "yes" "no")        → "yes"

(if (zero? x)
    100
    (/ 1 x))

(if (integer? x)
    (+ x 1)
    0)
```

**Important:** Only `#f` is falsy. Zero (as an integer) is truthy!

```scheme
(if 0 "yes" "no")               → "yes"
(if -1 "yes" "no")              → "yes"
(if #f "yes" "no")              → "no"
```

### Nested Conditionals

```scheme
(if (> x 10)
    (if (< x 20) "medium" "large")
    "small")

(if (> x 0)
    (if (> x 100) "huge" "positive")
    (if (= x 0) "zero" "negative"))
```

## Functions and Primitives

Primitives are built-in functions that perform operations. All primitives use **prefix notation** (function call syntax).

### Unary Primitives (1 argument)

```scheme
(prim arg)
```

Examples:

```scheme
(add1 5)                        → 6
(sub1 10)                       → 9
(zero? 0)                       → #t
(integer? 42)                   → #t
(boolean? #t)                   → #t
(char? #\A)                     → #t
(null? ())                      → #t
(integer->char 65)              → #\A
(char->integer #\A)             → 65
(car (cons 5 10))               → 5
(cdr (cons 5 10))               → 10
```

### Binary Primitives (2 arguments)

```scheme
(prim arg1 arg2)
```

Arithmetic:
```scheme
(+ 5 3)                         → 8
(- 10 4)                        → 6
(* 3 4)                         → 12
```

Comparison:
```scheme
(= 5 5)                         → #t
(< 3 5)                         → #t
(> 5 3)                         → #t
(<= 5 5)                        → #t
(>= 5 5)                        → #t
(char=? #\A #\B)               → #f
(char<? #\A #\B)               → #t
```

Heap operations:
```scheme
(cons 5 10)                     → pair
```

## Variables and Scoping

### Local Variables with `let`

Introduce a new variable within a scope:

```scheme
(let (variable init-expr)
  body-expr)
```

**Semantics:**
1. Evaluate `init-expr` to get a value
2. Bind `variable` to that value
3. Evaluate `body-expr` with variable in scope
4. Return the result of `body-expr`

**Examples:**

```scheme
; Simple binding
(let (x 5) x)                   → 5

; Use variable in expression
(let (x 10) (+ x 5))            → 15

; Variable shadows outer scope (if any)
(let (x 5) (+ x 3))             → 8

; Nested let expressions
(let (x 5)
  (let (y 10)
    (+ x y)))                   → 15

; Use let in conditionals
(let (x 5)
  (if (> x 3)
      (* x 2)
      (- x 1)))                 → 10
```

**Variable Scope:**
- Variable is only visible in `body-expr`
- Variable cannot be used in `init-expr`
- Variable shadows any outer variables with same name
- Scope ends after `body-expr` completes

```scheme
(let (x 5)
  (+ x 1))              → 6

(+ x 1)                 → ERROR: x is undefined (outside let scope)
```

### Nested Scopes

Create multiple levels of variables:

```scheme
(let (x 5)
  (let (y 10)
    (let (z 3)
      (+ x (+ y z)))))         → 18  (5 + 10 + 3)
```

Inner variables can use outer variables:

```scheme
(let (x 5)
  (let (y (+ x 10))             ; y = 5 + 10 = 15
    (+ x y)))                   → 20  (5 + 15)
```

## Heap and Data Structures

### Pairs with `cons`, `car`, `cdr`

Create and access pairs (2-element tuples) on the heap:

#### `cons` - Create a pair

```scheme
(cons car-value cdr-value)
```

Allocates 8 bytes on the heap:

```scheme
(cons 5 10)                     → pair with car=5, cdr=10
(cons #t #f)                    → pair with car=#t, cdr=#f
(cons 1 (cons 2 3))             → nested pair
```

#### `car` - Get first element

```scheme
(car pair)
```

Extracts the first element (car) of a pair:

```scheme
(car (cons 5 10))               → 5
(car (cons #t #f))              → #t
(car (cons 1 (cons 2 3)))       → 1
```

#### `cdr` - Get second element

```scheme
(cdr pair)
```

Extracts the second element (cdr) of a pair:

```scheme
(cdr (cons 5 10))               → 10
(cdr (cons #t #f))              → #f
(cdr (cons 1 (cons 2 3)))       → (cons 2 3)
```

#### Building Lists

Use nested `cons` to build list-like structures:

```scheme
(cons 1 (cons 2 (cons 3 ())))   → list [1, 2, 3]

; Access elements
(car (cdr (cons 1 (cons 2 3)))) → 2
```

## Examples

### Example 1: Simple Arithmetic

```scheme
return 2 + 3 * 4;               → 14 (exit code: 56)
```

Steps:
1. Multiplication: 3 * 4 = 12
2. Addition: 2 + 12 = 14
3. Tag: 14 << 2 = 56

### Example 2: Using Variables

```scheme
return (let (x 5)
         (let (y 3)
           (+ x y)));           → 8 (exit code: 32)
```

Steps:
1. Bind x = 5
2. Bind y = 3
3. Evaluate: 5 + 3 = 8
4. Tag: 8 << 2 = 32

### Example 3: Conditional Logic

```scheme
return (let (age 25)
         (if (>= age 18)
             1
             0));               → 1 (exit code: 4)
```

Steps:
1. Bind age = 25
2. Test: 25 >= 18 → #t
3. Return consequent: 1
4. Tag: 1 << 2 = 4

### Example 4: Type Checking

```scheme
return (let (x 42)
         (if (integer? x)
             (+ x 1)
             0));               → 43 (exit code: 172)
```

### Example 5: Working with Pairs

```scheme
return (let (p (cons 10 20))
         (+ (car p) (cdr p))); → 30 (exit code: 120)
```

Steps:
1. Create pair: car=10, cdr=20
2. Extract: car=10, cdr=20
3. Add: 10 + 20 = 30
4. Tag: 30 << 2 = 120

### Example 6: Complex Nested Structure

```scheme
return (let (x 5)
         (let (y 10)
           (if (> (+ x y) 10)
               (cons x y)
               0)));           → heap pointer (exit code depends on memory)
```

## Common Patterns

### Check and Compute

```scheme
(let (x 42)
  (if (integer? x)
      (+ x 1)
      0))
```

### Conditional Binding

```scheme
(if (> x 10)
    (let (y (- x 5)) y)
    x)
```

### Building Pairs

```scheme
(let (x 1)
  (let (y 2)
    (cons x y)))
```

## Limitations and Notes

- **No lambdas/functions yet:** Cannot define custom functions
- **No mutable operations:** Cannot change values after creation
- **No strings:** Character type exists but not full strings
- **No vectors/arrays:** Only pairs available for data structures
- **Limited scoping:** Only let for variables, no global scope yet
- **32-bit only:** x86-32 architecture, 30-bit integer values due to tagging
