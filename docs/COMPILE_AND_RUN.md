# Compiling and Running Programs

This guide explains each step of the compilation and execution pipeline for both RTE and CTE modes.

## Full Workflow Overview

```
Source Code (.c)
    ↓
Compiler (compiler-rte or compiler-cte)
    ↓
Assembly Code (.s)
    ↓
Assembler (as)
    ↓
Object File (.o)
    ↓
Linker (ld)
    ↓
Executable Binary
    ↓
Run & Check Exit Code
```

## Step-by-Step: RTE Mode

### Step 1: Compile to Assembly

```bash
build/compiler-rte examples/main.c
```

**What happens:**
- Reads your Scheme source code from `examples/main.c`
- Parses the expression
- Generates x86 assembly instructions for **runtime** evaluation
- Writes assembly to `out/output.s`

**Example input** (`examples/main.c`):
```scheme
return 2 + 3 * 4;
```

**Example output** (`out/output.s`):
```asm
    .globl main
main:
    movl $16, %eax       # Load 4 (tagged: 4 << 2 = 16)
    movl %eax, -4(%esp)  # Save to stack
    movl $12, %eax       # Load 3 (tagged: 3 << 2 = 12)
    movl -4(%esp), %ecx  # Load 4 from stack
    imull %ecx, %eax     # Multiply: 3 * 4 = 12
    sarl $2, %eax        # Fix double tagging (3*4 was tagged twice)
    movl %eax, -4(%esp)  # Save result (48 = 12 << 2)
    movl $8, %eax        # Load 2 (tagged: 2 << 2 = 8)
    addl -4(%esp), %eax  # Add: 2 + 12 = 14
    movl %eax, %ebx      # Move result to %ebx (exit code)
    movl $1, %eax        # Load syscall number for exit (1)
    int $0x80            # Call kernel to exit with code in %ebx
```

**Key points:**
- All computation is explicit in assembly
- 12 instructions total for this expression
- The result (14) is tagged as a fixnum: `14 << 2 = 56`

---

### Step 2: Assemble to Object Code

```bash
as --32 out/output.s -o out/output.o
```

**Breaking down the command:**
- `as` - The GNU assembler (part of binutils)
- `--32` - Generate 32-bit x86 code (not 64-bit)
- `out/output.s` - Input assembly file
- `-o out/output.o` - Output object file

**What happens:**
- Reads assembly instructions
- Translates each instruction to machine code (bytes)
- Creates object file with relocation information
- Produces `out/output.o` (binary but not yet executable)

**Example translation:**
```
Assembly:  movl $8, %eax
Machine:   b8 08 00 00 00  (5 bytes)

Assembly:  addl -4(%esp), %eax
Machine:   03 44 24 fc     (4 bytes)
```

**Object file contents:**
- Machine code for all instructions
- Symbol table (references to `main`)
- Relocation information (for linker)
- Section headers (code, data, etc.)

---

### Step 3: Link to Executable

```bash
ld -m elf_i386 out/output.o -o out/program
```

**Breaking down the command:**
- `ld` - The GNU linker (part of binutils)
- `-m elf_i386` - Target 32-bit x86 ELF format
- `out/output.o` - Input object file
- `-o out/program` - Output executable file

**What happens:**
- Reads object file with machine code
- Resolves symbol references (e.g., `main` entry point)
- Sets up entry point to start at `main`
- Creates executable file that OS can run
- Produces `out/program` (ready to execute)

**Key responsibilities:**
- Assigns memory addresses to code sections
- Resolves symbol table entries
- Creates ELF header (executable file format)
- Sets executable permissions

---

### Step 4: Execute the Program

```bash
./out/program
```

**What happens:**
1. OS loads `out/program` from disk
2. Reads ELF header to find entry point (`main`)
3. Executes machine instructions starting at `main`
4. Runs all 12 instructions from RTE mode
5. Instruction `int $0x80` triggers exit syscall
6. Program terminates with exit code stored in `%ebx`

**Execution trace:**
```
Instruction 1:  movl $16, %eax        → %eax = 16
Instruction 2:  movl %eax, -4(%esp)   → stack[0] = 16
Instruction 3:  movl $12, %eax        → %eax = 12
Instruction 4:  movl -4(%esp), %ecx   → %ecx = 16
Instruction 5:  imull %ecx, %eax      → %eax = 12 * 16 = 192
Instruction 6:  sarl $2, %eax         → %eax = 192 >> 2 = 48
Instruction 7:  movl %eax, -4(%esp)   → stack[0] = 48
Instruction 8:  movl $8, %eax         → %eax = 8
Instruction 9:  addl -4(%esp), %eax   → %eax = 8 + 48 = 56
Instruction 10: movl %eax, %ebx       → %ebx = 56 (exit code!)
Instruction 11: movl $1, %eax         → %eax = 1 (exit syscall)
Instruction 12: int $0x80             → Exit with code 56
```

---

### Step 5: Check the Exit Code

```bash
echo $?
```

**What happens:**
- Shell variable `$?` contains the exit code from last command
- Prints `56` (which is `14 << 2`)

**Why tagged?**
- Our type system encodes values in the low 2 bits
- `14` (mathematical result) becomes `56` (tagged fixnum)
- To get original value: `56 >> 2 = 14`

---

## Full RTE Workflow in One Command

```bash
build/compiler-rte examples/main.c && as --32 out/output.s -o out/output.o && ld -m elf_i386 out/output.o -o out/program && ./out/program; echo $?
```

This runs all steps and shows the exit code.

---

## Step-by-Step: CTE Mode

### Compilation: CTE (Optimized)

```bash
build/compiler-cte -O examples/main.c
```

**Command breakdown:**
- `build/compiler-cte` - Compiler in CTE mode
- `-O` - Enable optimization (compile-time evaluation)

**What happens:**
- Reads source code from `examples/main.c`
- Parses the expression
- **Key difference**: Detects that expression contains only constants
- **Evaluates at compile-time**: `2 + 3 * 4 = 14`
- Generates assembly with **only the final result**
- Writes optimized assembly to `out/output.s`

**Example input** (same as RTE):
```scheme
return 2 + 3 * 4;
```

**Example output** (drastically different):
```asm
    .globl main
main:
    movl $56, %eax       # Direct result: 14 << 2 = 56
    movl %eax, %ebx      # Move to exit code
    movl $1, %eax        # Load exit syscall number
    int $0x80            # Call kernel to exit
```

**Key differences from RTE:**
- Only 4 instructions (vs 12 in RTE)
- No stack operations
- No multiplication or addition at runtime
- Result is pre-computed and hardcoded
- 67% code size reduction

---

### Rest of Pipeline: Same as RTE

Once CTE generates assembly, the rest is **identical**:

```bash
# Assemble (same as RTE)
as --32 out/output.s -o out/output.o

# Link (same as RTE)
ld -m elf_i386 out/output.o -o out/program

# Execute (same as RTE)
./out/program
echo $?  # Still prints 56
```

---

## Comparison: RTE vs CTE

| Aspect | RTE | CTE |
|--------|-----|-----|
| **Compiler** | `compiler-rte` | `compiler-cte -O` |
| **When expression evaluated** | At runtime (step 4) | At compile-time (step 1) |
| **Instructions generated** | 12 | 4 |
| **Code size** | Larger | 67% smaller |
| **Execution speed** | Slower (computation) | Faster (no computation) |
| **Assembly contains** | Computation logic | Direct result |
| **Assembler step** | Translates 12 instructions | Translates 4 instructions |
| **Linker step** | Links larger code | Links smaller code |
| **Run time** | Executes 12 instructions | Executes 4 instructions |

---

## What About Variables?

When variables are added (future feature), **only constants** are pre-computed:

Example:
```scheme
return x + 2 * 3;
```

**RTE mode:**
- Generates runtime code to load `x`
- Generates code to multiply 2 * 3
- Generates code to add results

**CTE mode (partial evaluation):**
- Pre-computes: `2 * 3 = 6` → `6 << 2 = 24`
- Generates code to load `x`
- Generates code to add `x` to pre-computed `24`
- Eliminates the 2*3 multiplication

---

## Building Different Modes

```bash
# Build RTE compiler only
make build-rte

# Build CTE compiler only
make build-cte

# Build both compilers
make build-all

# Test RTE mode
make test-rte

# Test CTE mode
make test-cte

# Test and compare both
make test-all
```

---

## Common Issues

### "as: unrecognized option '--32'"

Your system might have a newer assembler. Try:
```bash
as --32 out/output.s -o out/output.o
# or
gcc -m32 -c out/output.s -o out/output.o
```

### "ld: cannot find Scrt1.o"

You're using the linker without C runtime. Use our exact command:
```bash
ld -m elf_i386 out/output.o -o out/program
```

### Program doesn't execute

Ensure `out/` directory exists:
```bash
mkdir -p out/
```

Then recompile.

---

## Summary

**RTE Mode** (Development):
1. Compiler generates full computation logic
2. Assembler translates all instructions
3. Linker creates executable with computation code
4. Program runs all computation at runtime

**CTE Mode** (Production):
1. Compiler **pre-computes** constant expressions
2. Assembler translates fewer, simpler instructions
3. Linker creates smaller executable
4. Program runs with result already computed

**Key insight:** Both produce identical results, but CTE does the math once (during compilation) while RTE does it every time the program runs.
