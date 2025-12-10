#include "ast.h"
#include <stdlib.h>

Expr* expr_fixnum(int value) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_FIXNUM;
    e->data.fixnum.value = value;
    return e;
}

Expr* expr_boolean(int value) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_BOOLEAN;
    e->data.boolean.value = value;
    return e;
}

Expr* expr_character(char value) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_CHARACTER;
    e->data.character.value = value;
    return e;
}

Expr* expr_empty_list(void) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_EMPTY_LIST;
    return e;
}

Expr* expr_unary_prim(UnaryPrimType op, Expr *operand) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_UNARY_PRIM;
    e->data.unary_prim.op = op;
    e->data.unary_prim.operand = operand;
    return e;
}

Expr* expr_binary_prim(BinaryPrimType op, Expr *operand1, Expr *operand2) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_BINARY_PRIM;
    e->data.binary_prim.op = op;
    e->data.binary_prim.operand1 = operand1;
    e->data.binary_prim.operand2 = operand2;
    return e;
}

void expr_free(Expr *expr) {
    if (!expr) return;
    
    switch (expr->type) {
        case EXPR_FIXNUM:
        case EXPR_BOOLEAN:
        case EXPR_CHARACTER:
        case EXPR_EMPTY_LIST:
            break;
        case EXPR_UNARY_PRIM:
            expr_free(expr->data.unary_prim.operand);
            break;
        case EXPR_BINARY_PRIM:
            expr_free(expr->data.binary_prim.operand1);
            expr_free(expr->data.binary_prim.operand2);
            break;
    }
    free(expr);
}
