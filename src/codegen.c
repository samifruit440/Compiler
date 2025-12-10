#include "codegen.h"
#include "tags.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Environment for tracking variable bindings to stack offsets */
typedef struct EnvEntry {
    char *name;
    int stack_offset;  /* Negative offset from %esp, e.g., -4, -8, -12 */
    struct EnvEntry *next;
} EnvEntry;

typedef struct {
    EnvEntry *bindings;
} Environment;

/* Global label counter for generating unique labels */
static int label_counter = 0;

/* Create a new environment */
static Environment* env_create(void) {
    Environment *env = malloc(sizeof(Environment));
    env->bindings = NULL;
    return env;
}

/* Extend environment with a new variable binding (creates a new environment) */
static Environment* env_extend(Environment *env, const char *name, int stack_offset) {
    Environment *new_env = malloc(sizeof(Environment));
    EnvEntry *entry = malloc(sizeof(EnvEntry));
    entry->name = malloc(strlen(name) + 1);
    strcpy(entry->name, name);
    entry->stack_offset = stack_offset;
    entry->next = env ? env->bindings : NULL;
    new_env->bindings = entry;
    return new_env;
}

/* Look up a variable in the environment
   Returns the stack offset, or -999 if not found */
static int env_lookup(Environment *env, const char *name) {
    if (!env) return -999;
    for (EnvEntry *entry = env->bindings; entry; entry = entry->next) {
        if (strcmp(entry->name, name) == 0) {
            return entry->stack_offset;
        }
    }
    return -999;
}

/* Free an environment */
static void env_free(Environment *env) {
    if (!env) return;
    for (EnvEntry *entry = env->bindings; entry; ) {
        EnvEntry *next = entry->next;
        free(entry->name);
        free(entry);
        entry = next;
    }
    free(env);
}

/* Generate a unique label */
static void emit_label(FILE *out, const char *label) {
    fprintf(out, "%s:\n", label);
}

static void emit_jmp(FILE *out, const char *label) {
    fprintf(out, "    jmp %s\n", label);
}

static void emit_je(FILE *out, const char *label) {
    fprintf(out, "    je %s\n", label);
}

/* Generate a unique label name */
static char* unique_label(void) {
    static char labels[100][32];
    static int label_count = 0;
    if (label_count >= 100) {
        fprintf(stderr, "Error: Too many labels generated\n");
        exit(1);
    }
    snprintf(labels[label_count], sizeof(labels[label_count]), ".L%d", label_counter++);
    return labels[label_count++];
}

/* Global compilation mode (default: runtime evaluation) */
EvalMode compilation_mode = MODE_RTE;

/* Forward declarations */
static int emit_expr(FILE *out, Expr *expr, int si, Environment *env);
static int is_constant_expr(Expr *expr);


/* Evaluate constant expressions at compile time */
static int eval_expr(Expr *expr) {
    if (!expr) {
        fprintf(stderr, "Error: NULL expression\n");
        exit(1);
    }
    
    switch (expr->type) {
        case EXPR_FIXNUM:
            return tag_fixnum(expr->data.fixnum.value);
        case EXPR_BOOLEAN:
            return expr->data.boolean.value ? (bool_tag | 0x20) : bool_tag;
        case EXPR_CHARACTER:
            return char_tag | (expr->data.character.value << 8);
        case EXPR_EMPTY_LIST:
            return empty_list_tag;
        case EXPR_UNARY_PRIM: {
            int operand = eval_expr(expr->data.unary_prim.operand);
            switch (expr->data.unary_prim.op) {
                case PRIM_ADD1:
                    return operand + 4;
                case PRIM_SUB1:
                    return operand - 4;
                case PRIM_ZERO_P:
                    return (operand == 0) ? (bool_tag | 0x20) : bool_tag;
                case PRIM_INTEGER_P:
                    return ((operand & 3) == 0) ? (bool_tag | 0x20) : bool_tag;
                case PRIM_BOOLEAN_P:
                    return ((operand & 0x3F) == 0x1F) ? (bool_tag | 0x20) : bool_tag;
                case PRIM_NULL_P:
                    return (operand == empty_list_tag) ? (bool_tag | 0x20) : bool_tag;
                case PRIM_CHAR_P:
                    return ((operand & 0xFF) == char_tag) ? (bool_tag | 0x20) : bool_tag;
                case PRIM_INTEGER_TO_CHAR:
                    return (operand << 6) | char_tag;
                case PRIM_CHAR_TO_INTEGER:
                    return (operand >> 8) << 2;
                default:
                    fprintf(stderr, "Error: Unknown unary primitive in eval\n");
                    exit(1);
            }
        }
        case EXPR_BINARY_PRIM: {
            int left = eval_expr(expr->data.binary_prim.operand1);
            int right = eval_expr(expr->data.binary_prim.operand2);
            switch (expr->data.binary_prim.op) {
                case PRIM_PLUS:
                    return left + right;
                case PRIM_MINUS:
                    return left - right;
                case PRIM_MULTIPLY:
                    return (left * right) >> 2;
                case PRIM_EQUALS:
                    return (left == right) ? (bool_tag | 0x20) : bool_tag;
                case PRIM_LESS:
                    return (left < right) ? (bool_tag | 0x20) : bool_tag;
                case PRIM_GREATER:
                    return (left > right) ? (bool_tag | 0x20) : bool_tag;
                case PRIM_LESS_EQUAL:
                    return (left <= right) ? (bool_tag | 0x20) : bool_tag;
                case PRIM_GREATER_EQUAL:
                    return (left >= right) ? (bool_tag | 0x20) : bool_tag;
                case PRIM_CHAR_EQUAL:
                    return (left == right) ? (bool_tag | 0x20) : bool_tag;
                case PRIM_CHAR_LESS:
                    return (left < right) ? (bool_tag | 0x20) : bool_tag;
                default:
                    fprintf(stderr, "Error: Unknown binary primitive in eval\n");
                    exit(1);
            }
        }
        default:
            fprintf(stderr, "Error: Unknown expression type in eval_expr\n");
            exit(1);
    }
}

/* Check if an expression is fully constant (no variables) */
static int is_constant_expr(Expr *expr) {
    if (!expr) return 0;
    
    switch (expr->type) {
        case EXPR_FIXNUM:
        case EXPR_BOOLEAN:
        case EXPR_CHARACTER:
        case EXPR_EMPTY_LIST:
            return 1;
        case EXPR_UNARY_PRIM:
            return is_constant_expr(expr->data.unary_prim.operand);
        case EXPR_BINARY_PRIM:
            return is_constant_expr(expr->data.binary_prim.operand1) &&
                   is_constant_expr(expr->data.binary_prim.operand2);
        case EXPR_LET:
        case EXPR_IF:
        case EXPR_CONS:
        case EXPR_CAR:
        case EXPR_CDR:
        case EXPR_VARIABLE:
            /* These require runtime evaluation or environment, not constant */
            return 0;
        default:
            return 0;
    }
}

/* Emit code for a unary primitive */
static void emit_unary_prim(FILE *out, UnaryPrimType prim, int si) {
    (void)si; /* Unused for unary ops */
    switch (prim) {
        case PRIM_ADD1:
            fprintf(out, "    addl $4, %%eax\n");
            break;
        case PRIM_SUB1:
            fprintf(out, "    subl $4, %%eax\n");
            break;
        case PRIM_ZERO_P:
            /* Compare eax with 0, store boolean result */
            fprintf(out, "    cmpl $0, %%eax\n");
            fprintf(out, "    sete %%al\n");
            fprintf(out, "    movzbl %%al, %%eax\n");
            fprintf(out, "    sall $6, %%eax\n");
            fprintf(out, "    orl $0x3f, %%eax\n");
            break;
        case PRIM_INTEGER_P:
            /* Check if last 2 bits are 00b */
            fprintf(out, "    movl %%eax, %%ecx\n");
            fprintf(out, "    andl $3, %%ecx\n");
            fprintf(out, "    cmpl $0, %%ecx\n");
            fprintf(out, "    sete %%al\n");
            fprintf(out, "    movzbl %%al, %%eax\n");
            fprintf(out, "    sall $6, %%eax\n");
            fprintf(out, "    orl $0x3f, %%eax\n");
            break;
        case PRIM_BOOLEAN_P:
            /* Check if lower bits match boolean tag */
            fprintf(out, "    movl %%eax, %%ecx\n");
            fprintf(out, "    andl $0x3f, %%ecx\n");
            fprintf(out, "    cmpl $0x1f, %%ecx\n");
            fprintf(out, "    sete %%al\n");
            fprintf(out, "    movzbl %%al, %%eax\n");
            fprintf(out, "    sall $6, %%eax\n");
            fprintf(out, "    orl $0x3f, %%eax\n");
            break;
        case PRIM_NULL_P:
            /* Check if value is empty_list_tag (0x2F) */
            fprintf(out, "    cmpl $0x2f, %%eax\n");
            fprintf(out, "    sete %%al\n");
            fprintf(out, "    movzbl %%al, %%eax\n");
            fprintf(out, "    sall $6, %%eax\n");
            fprintf(out, "    orl $0x3f, %%eax\n");
            break;
        case PRIM_CHAR_P:
            /* Check if value has char tag (0x0F | (char << 8)) */
            fprintf(out, "    movl %%eax, %%ecx\n");
            fprintf(out, "    andl $0xff, %%ecx\n");
            fprintf(out, "    cmpl $0x0f, %%ecx\n");
            fprintf(out, "    sete %%al\n");
            fprintf(out, "    movzbl %%al, %%eax\n");
            fprintf(out, "    sall $6, %%eax\n");
            fprintf(out, "    orl $0x3f, %%eax\n");
            break;
        case PRIM_INTEGER_TO_CHAR:
            /* Shift left by 6 bits: (val << 6) | char_tag
               Result: (val << 8) | 0x0F */
            fprintf(out, "    sall $6, %%eax\n");
            fprintf(out, "    orl $0x0f, %%eax\n");
            break;
        case PRIM_CHAR_TO_INTEGER:
            /* Shift right by 8 bits, then remove char tag, retag as fixnum
               Result: (val >> 8) << 2 */
            fprintf(out, "    shrl $8, %%eax\n");
            fprintf(out, "    sall $2, %%eax\n");
            break;
        default:
            fprintf(stderr, "Error: Unknown unary primitive\n");
            exit(1);
    }
}

/* Emit code for a binary primitive
   %eax = left operand, stack[si] = right operand */
static void emit_binary_prim(FILE *out, BinaryPrimType prim, int si) {
    switch (prim) {
        case PRIM_PLUS:
            fprintf(out, "    addl %d(%%esp), %%eax\n", si);
            break;
        case PRIM_MINUS:
            /* %eax = left operand, stack[si] = right operand
               We want: left - right = %eax - stack[si] */
            fprintf(out, "    subl %d(%%esp), %%eax\n", si);
            break;
        case PRIM_MULTIPLY:
            /* %eax = left operand (tagged), stack[si] = right operand (tagged)
               Both are shifted by 2, so result will be shifted by 4
               We need to untag one: (%eax >> 2) * (%ecx << 2) = %eax * %ecx >> 2 */
            fprintf(out, "    movl %d(%%esp), %%ecx\n", si);
            fprintf(out, "    imull %%ecx, %%eax\n");
            fprintf(out, "    sarl $2, %%eax\n");
            break;
        case PRIM_EQUALS:
            fprintf(out, "    cmpl %%eax, %d(%%esp)\n", si + 4);
            fprintf(out, "    sete %%al\n");
            fprintf(out, "    movzbl %%al, %%eax\n");
            fprintf(out, "    sall $6, %%eax\n");
            fprintf(out, "    orl $0x1f, %%eax\n");
            break;
        case PRIM_LESS:
            /* left < right: cmp left right; setl */
            fprintf(out, "    cmpl %%eax, %d(%%esp)\n", si + 4);
            fprintf(out, "    setg %%al\n");  /* right > left means left < right */
            fprintf(out, "    movzbl %%al, %%eax\n");
            fprintf(out, "    sall $6, %%eax\n");
            fprintf(out, "    orl $0x1f, %%eax\n");
            break;
        case PRIM_GREATER:
            /* left > right: cmp left right; setg */
            fprintf(out, "    cmpl %%eax, %d(%%esp)\n", si + 4);
            fprintf(out, "    setl %%al\n");  /* right < left means left > right */
            fprintf(out, "    movzbl %%al, %%eax\n");
            fprintf(out, "    sall $6, %%eax\n");
            fprintf(out, "    orl $0x1f, %%eax\n");
            break;
        case PRIM_LESS_EQUAL:
            fprintf(out, "    cmpl %%eax, %d(%%esp)\n", si + 4);
            fprintf(out, "    setge %%al\n");  /* right >= left means left <= right */
            fprintf(out, "    movzbl %%al, %%eax\n");
            fprintf(out, "    sall $6, %%eax\n");
            fprintf(out, "    orl $0x1f, %%eax\n");
            break;
        case PRIM_GREATER_EQUAL:
            fprintf(out, "    cmpl %%eax, %d(%%esp)\n", si + 4);
            fprintf(out, "    setle %%al\n");  /* right <= left means left >= right */
            fprintf(out, "    movzbl %%al, %%eax\n");
            fprintf(out, "    sall $6, %%eax\n");
            fprintf(out, "    orl $0x3f, %%eax\n");
            break;
        case PRIM_CHAR_EQUAL:
            fprintf(out, "    cmpl %%eax, %d(%%esp)\n", si);
            fprintf(out, "    sete %%al\n");
            fprintf(out, "    movzbl %%al, %%eax\n");
            fprintf(out, "    sall $6, %%eax\n");
            fprintf(out, "    orl $0x3f, %%eax\n");
            break;
        case PRIM_CHAR_LESS:
            fprintf(out, "    cmpl %%eax, %d(%%esp)\n", si);
            fprintf(out, "    setl %%al\n");
            fprintf(out, "    movzbl %%al, %%eax\n");
            fprintf(out, "    sall $6, %%eax\n");
            fprintf(out, "    orl $0x3f, %%eax\n");
            break;
        default:
            fprintf(stderr, "Error: Unknown binary primitive\n");
            exit(1);
    }
}

/* Recursively emit code for an expression, returning its value in %eax
   si: stack index (for saving temporary values on stack)
   env: environment for variable bindings
   Returns: updated stack index */
static int emit_expr(FILE *out, Expr *expr, int si, Environment *env) {
    if (!expr) {
        fprintf(stderr, "Error: NULL expression in code generation\n");
        exit(1);
    }
    
    /* Compile-Time Evaluation: if enabled and expression is constant, evaluate now */
    if (compilation_mode == MODE_CTE && is_constant_expr(expr)) {
        int val = eval_expr(expr);
        fprintf(out, "    movl $%d, %%eax\n", val);
        return si;
    }
    
    /* Runtime Evaluation: generate assembly code */
    switch (expr->type) {
        case EXPR_FIXNUM:
        case EXPR_BOOLEAN:
        case EXPR_CHARACTER:
        case EXPR_EMPTY_LIST: {
            /* For immediate constants, evaluate and move to eax */
            int val = eval_expr(expr);
            fprintf(out, "    movl $%d, %%eax\n", val);
            return si;
        }
        case EXPR_VARIABLE: {
            /* Load variable from stack */
            int offset = env_lookup(env, expr->data.variable.name);
            if (offset == -999) {
                fprintf(stderr, "Error: Undefined variable: %s\n", expr->data.variable.name);
                exit(1);
            }
            fprintf(out, "    movl %d(%%esp), %%eax\n", offset);
            return si;
        }
        case EXPR_UNARY_PRIM: {
            /* Emit code for the operand first */
            si = emit_expr(out, expr->data.unary_prim.operand, si, env);
            /* Then emit the unary operation */
            emit_unary_prim(out, expr->data.unary_prim.op, si);
            return si;
        }
        case EXPR_BINARY_PRIM: {
            /* Evaluate right operand first */
            si = emit_expr(out, expr->data.binary_prim.operand2, si, env);
            /* Save it on the stack at current si */
            fprintf(out, "    movl %%eax, %d(%%esp)\n", si);
            si -= 4;
            /* Evaluate left operand */
            si = emit_expr(out, expr->data.binary_prim.operand1, si, env);
            /* Emit the binary operation
               %eax has left operand, stack[si+4] has right operand */
            emit_binary_prim(out, expr->data.binary_prim.op, si + 4);
            si += 4;
            return si;
        }
        case EXPR_LET: {
            /* Evaluate the initialization expression */
            si = emit_expr(out, expr->data.let_expr.init, si, env);
            /* Save it on the stack at current si */
            fprintf(out, "    movl %%eax, %d(%%esp)\n", si);
            /* Create extended environment with new binding */
            Environment *new_env = env_extend(env, expr->data.let_expr.name, si);
            /* Evaluate body in extended environment */
            si -= 4;
            si = emit_expr(out, expr->data.let_expr.body, si, new_env);
            /* Free the extended environment (but keep original unchanged) */
            env_free(new_env);
            return si;
        }
        case EXPR_IF: {
            /* Generate labels for branches */
            char *L_false = unique_label();
            char *L_end = unique_label();
            
            /* Evaluate test expression */
            si = emit_expr(out, expr->data.if_expr.test, si, env);
            
            /* Compare result to false (0x1F) */
            fprintf(out, "    cmpl $0x1f, %%eax\n");
            emit_je(out, L_false);
            
            /* Consequent branch */
            si = emit_expr(out, expr->data.if_expr.consequent, si, env);
            emit_jmp(out, L_end);
            
            /* Alternate branch */
            emit_label(out, L_false);
            si = emit_expr(out, expr->data.if_expr.alternate, si, env);
            
            /* End label */
            emit_label(out, L_end);
            return si;
        }
        case EXPR_CONS: {
            /* Evaluate car and save on stack */
            si = emit_expr(out, expr->data.cons.car_expr, si, env);
            fprintf(out, "    movl %%eax, %d(%%esp)\n", si);
            si -= 4;
            /* Evaluate cdr and save on stack */
            si = emit_expr(out, expr->data.cons.cdr_expr, si, env);
            fprintf(out, "    movl %%eax, %d(%%esp)\n", si);
            si -= 4;
            /* At this point: stack[si+8] = car, stack[si+4] = cdr
               Return a tagged pointer to the pair on the stack: (si | 0x01) */
            fprintf(out, "    movl %%esp, %%eax\n");
            fprintf(out, "    addl $%d, %%eax\n", si + 4);
            fprintf(out, "    orl $1, %%eax\n");
            return si;
        }
        case EXPR_CAR: {
            /* Evaluate pair expression */
            si = emit_expr(out, expr->data.car.pair, si, env);
            /* Remove pair tag by subtracting 1 to get stack address */
            fprintf(out, "    subl $1, %%eax\n");
            /* Load car from the pair (which is at offset 4 from the stack address) */
            fprintf(out, "    movl 4(%%eax), %%eax\n");
            return si;
        }
        case EXPR_CDR: {
            /* Evaluate pair expression */
            si = emit_expr(out, expr->data.cdr.pair, si, env);
            /* Remove pair tag by subtracting 1 to get stack address */
            fprintf(out, "    subl $1, %%eax\n");
            /* Load cdr from the pair (which is at offset 0 from the stack address) */
            fprintf(out, "    movl (%%eax), %%eax\n");
            return si;
        }
        default:
            fprintf(stderr, "Error: Unknown expression type in code generation\n");
            exit(1);
    }
}

void emit_program(FILE *out, Expr *expr) {
    fprintf(out, "    .text\n");
    fprintf(out, "    .globl _start\n");
    fprintf(out, "_start:\n");
    
    /* Initialize heap pointer in %esi to a reasonable location
       (We'd normally receive this from the OS, but for testing we use a hardcoded address) */
    fprintf(out, "    movl $0x100000, %%esi  # Heap pointer\n");
    
    /* Create initial environment (empty for top-level) */
    Environment *env = env_create();
    
    /* Emit code for the expression, stack index starts at -4 (first temporary at -4(%esp)) */
    emit_expr(out, expr, -4, env);
    
    /* Free environment */
    env_free(env);
    
    /* Result is in %eax, move to %ebx and exit */
    fprintf(out, "    movl %%eax, %%ebx     # return value\n");
    fprintf(out, "    movl $1, %%eax      # exit syscall\n");
    fprintf(out, "    int $0x80\n");
}
