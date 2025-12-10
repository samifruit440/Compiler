#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* Test mode: 0 = RTE, 1 = CTE */
#ifndef TEST_MODE
#define TEST_MODE 0
#endif

/* Type tags - must match src/tags.h */
#define char_tag 0x0F
#define bool_tag 0x1F
#define empty_list_tag 0x2F

const char *compiler_path = TEST_MODE ? "build/compiler-cte -O" : "build/compiler-rte";

int test_count = 0;
int passed_count = 0;

void test_expr(const char *expr, int expected, const char *type_name) {
    test_count++;
    
    char src_file[64];
    char asm_file[64];
    char obj_file[64];
    char exe_file[64];
    char cmd[256];
    
    sprintf(src_file, "test_output/test_%d.c", test_count);
    sprintf(asm_file, "test_output/test_%d.s", test_count);
    sprintf(obj_file, "test_output/test_%d.o", test_count);
    sprintf(exe_file, "test_output/test_%d", test_count);
    
    /* Create source file */
    FILE *src = fopen(src_file, "w");
    fprintf(src, "return %s;\n", expr);
    fclose(src);
    
    /* Compile with our compiler - it outputs to out/output.s, so copy it */
    sprintf(cmd, "%s %s > /dev/null 2>&1 && cp out/output.s %s", 
            compiler_path, src_file, asm_file);
    if (system(cmd) != 0) {
        printf("FAIL: Could not compile '%s'\n", expr);
        remove(src_file);
        remove(asm_file);
        return;
    }
    
    /* Assemble */
    sprintf(cmd, "as --32 %s -o %s 2>/dev/null", asm_file, obj_file);
    if (system(cmd) != 0) {
        printf("FAIL: Could not assemble '%s'\n", expr);
        remove(src_file);
        remove(asm_file);
        return;
    }
    
    /* Link */
    sprintf(cmd, "ld -m elf_i386 %s -o %s 2>/dev/null", obj_file, exe_file);
    if (system(cmd) != 0) {
        printf("FAIL: Could not link '%s'\n", expr);
        remove(src_file);
        remove(asm_file);
        remove(obj_file);
        return;
    }
    
    /* Run and check exit code */
    sprintf(cmd, "./%s", exe_file);
    int status = system(cmd);
    int exit_code = WEXITSTATUS(status);
    
    /* Clean up */
    remove(src_file);
    remove(obj_file);
    remove(exe_file);
    
    if (exit_code == (expected & 0xFF)) {
        /* Calculate untagged value for display (only for fixnums) */
        if (strcmp(type_name, "fixnum") == 0) {
            int untagged = expected >> 2;
            printf("PASS: return %s; → Expected: %d (%d as %s)\n", expr, exit_code, untagged, type_name);
        } else {
            printf("PASS: return %s; → Expected: %d (as %s)\n", expr, exit_code, type_name);
        }
        passed_count++;
    } else {
        printf("FAIL: return %s; expected %d but got %d\n", expr, expected, exit_code);
    }
}

int main() {
    printf("Running compiler tests...\n\n");
    
    /* Create test output directory */
    system("mkdir -p test_output");
    
    /* ========================================
       SECTION 1: Immediate Values
       ======================================== */
    printf("--- Section 1: Immediate Values ---\n");
    
    /* Test integers - fixnums are tagged: value << 2 */
    test_expr("0", 0 << 2, "fixnum");           /* 0 */
    test_expr("1", 1 << 2, "fixnum");           /* 4 */
    test_expr("42", 42 << 2, "fixnum");         /* 168 */
    test_expr("127", 127 << 2, "fixnum");       /* 508 */
    
    /* Test booleans */
    /* #t is tagged as 0x3F = 63 */
    test_expr("#t", 63, "boolean");
    /* #f is tagged as 0x1F = 31 */
    test_expr("#f", 31, "boolean");
    
    /* Test characters */
    /* #\A is char_tag (0x0F) | (65 << 8) */
    test_expr("#\\A", char_tag | (65 << 8), "character");
    /* #\space is char_tag (0x0F) | (32 << 8) */
    test_expr("#\\space", char_tag | (32 << 8), "character");
    
    /* Test empty list */
    /* () is empty_list_tag = 0x2F = 47 */
    test_expr("()", 47, "empty list");
    
    /* ========================================
       SECTION 2: Arithmetic Operators
       ======================================== */
    printf("\n--- Section 2: Arithmetic Operators ---\n");
    
    /* Test addition */
    test_expr("10 + 5", 15 << 2, "fixnum");     /* 60 */
    test_expr("100 + 55", 155 << 2, "fixnum");  /* 620 */
    
    /* Test subtraction */
    test_expr("50 - 20", 30 << 2, "fixnum");    /* 120 */
    test_expr("42 - 42", 0 << 2, "fixnum");     /* 0 */
    
    /* Test multiplication */
    test_expr("6 * 7", 42 << 2, "fixnum");      /* 168 */
    test_expr("10 * 10", 100 << 2, "fixnum");   /* 400 */
    
    /* Test operator precedence and grouping */
    test_expr("2 + 3 * 4", 14 << 2, "fixnum");  /* 56 (multiplication first) */
    test_expr("(10 + 5) * 2", 30 << 2, "fixnum");   /* 120 */
    test_expr("2 * (10 + 5)", 30 << 2, "fixnum");   /* 120 */
    test_expr("10 + 20 - 5", 25 << 2, "fixnum"); /* 100 */
    
    /* ========================================
       SECTION 3: Let Expressions (New Feature)
       ======================================== */
    printf("\n--- Section 3: Let Expressions ---\n");
    
    /* Simple let binding */
    test_expr("(let (x 5) x)", 5 << 2, "fixnum");  /* 20 */
    test_expr("(let (n 42) n)", 42 << 2, "fixnum"); /* 168 */
    
    /* Let with arithmetic */
    test_expr("(let (x 5) (+ x 3))", 8 << 2, "fixnum");     /* 32 */
    test_expr("(let (x 10) (* x 2))", 20 << 2, "fixnum");   /* 80 */
    test_expr("(let (x 7) (- x 2))", 5 << 2, "fixnum");     /* 20 */
    
    /* Let with infix arithmetic inside let body */
    test_expr("(let (x 3) x + 5)", 8 << 2, "fixnum");  /* 32 */
    
    /* ========================================
       SECTION 4: If Expressions (New Feature)
       ======================================== */
    printf("\n--- Section 4: If Expressions ---\n");
    
    /* If with boolean literals */
    test_expr("(if #t 10 5)", 10 << 2, "fixnum");  /* 40 (true branch) */
    test_expr("(if #f 10 5)", 5 << 2, "fixnum");   /* 20 (false branch) */
    
    /* If with nested ifs */
    test_expr("(if #t (if #t 10 5) 0)", 10 << 2, "fixnum");  /* 40 */
    test_expr("(if #t (if #f 10 5) 0)", 5 << 2, "fixnum");   /* 20 */
    
    /* ========================================
       SECTION 5: Heap Operations
       cons/car/cdr - WORKING! (finally)
       ======================================== */
    printf("\n--- Section 5: Heap Operations (cons/car/cdr) ---\n");
    
    /* Simple cons and car */
    test_expr("(car (cons 5 10))", 5 << 2, "fixnum");  /* 20 */
    
    /* Simple cons and cdr */
    test_expr("(cdr (cons 5 10))", 10 << 2, "fixnum"); /* 40 */
    
    /* Cons with different values */
    test_expr("(car (cons 42 99))", 42 << 2, "fixnum");  /* 168 */
    test_expr("(cdr (cons 42 99))", 99 << 2, "fixnum");  /* 396 */
    
    /* Cons with arithmetic */
    test_expr("(car (cons (+ 3 4) 10))", 7 << 2, "fixnum");  /* 28 */
    test_expr("(cdr (cons 5 (* 2 5)))", 10 << 2, "fixnum");  /* 40 */
    
    /* Cons with let bindings */
    test_expr("(let (x 5) (car (cons x 10)))", 5 << 2, "fixnum");   /* 20 */
    test_expr("(let (x 5) (cdr (cons x 10)))", 10 << 2, "fixnum");  /* 40 */
    
    /* Cons with if expressions */
    test_expr("(car (cons (if #t 5 10) 20))", 5 << 2, "fixnum");   /* 20 */
    test_expr("(cdr (cons (if #f 5 10) 20))", 20 << 2, "fixnum");  /* 80 */
    
    printf("\n========================================\n");
    printf("Tests: %d passed, %d failed, %d total\n", 
           passed_count, test_count - passed_count, test_count);
    printf("========================================\n");
    
    return (test_count == passed_count) ? 0 : 1;
}
