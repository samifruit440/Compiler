#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"

/* Evaluation mode: compile-time vs runtime evaluation */
typedef enum {
    MODE_RTE,  /* Runtime Evaluation: generates assembly for all operations */
    MODE_CTE   /* Compile-Time Evaluation: pre-computes constant expressions */
} EvalMode;

/* Global compilation mode (set by main.c based on command-line flags) */
extern EvalMode compilation_mode;

/* Generate x86 32-bit AT&T assembly from an AST expression */
void emit_program(FILE *out, Expr *expr);

#endif
