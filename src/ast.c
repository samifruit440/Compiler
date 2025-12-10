#include "ast.h"
#include <stdlib.h>
#include <string.h>

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

Expr* expr_variable(const char *name) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_VARIABLE;
    e->data.variable.name = malloc(strlen(name) + 1);
    strcpy(e->data.variable.name, name);
    return e;
}

Expr* expr_let(const char *name, Expr *init, Expr *body) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_LET;
    e->data.let_expr.name = malloc(strlen(name) + 1);
    strcpy(e->data.let_expr.name, name);
    e->data.let_expr.init = init;
    e->data.let_expr.body = body;
    return e;
}

Expr* expr_if(Expr *test, Expr *consequent, Expr *alternate) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_IF;
    e->data.if_expr.test = test;
    e->data.if_expr.consequent = consequent;
    e->data.if_expr.alternate = alternate;
    return e;
}

Expr* expr_cons(Expr *car_expr, Expr *cdr_expr) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_CONS;
    e->data.cons.car_expr = car_expr;
    e->data.cons.cdr_expr = cdr_expr;
    return e;
}

Expr* expr_car(Expr *pair) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_CAR;
    e->data.car.pair = pair;
    return e;
}

Expr* expr_cdr(Expr *pair) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_CDR;
    e->data.cdr.pair = pair;
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
        case EXPR_VARIABLE:
            free(expr->data.variable.name);
            break;
        case EXPR_LET:
            free(expr->data.let_expr.name);
            expr_free(expr->data.let_expr.init);
            expr_free(expr->data.let_expr.body);
            break;
        case EXPR_IF:
            expr_free(expr->data.if_expr.test);
            expr_free(expr->data.if_expr.consequent);
            expr_free(expr->data.if_expr.alternate);
            break;
        case EXPR_CONS:
            expr_free(expr->data.cons.car_expr);
            expr_free(expr->data.cons.cdr_expr);
            break;
        case EXPR_CAR:
            expr_free(expr->data.car.pair);
            break;
        case EXPR_CDR:
            expr_free(expr->data.cdr.pair);
            break;
    }
    free(expr);
}
