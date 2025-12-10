#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Token current_token;

/* Helper functions to map identifier strings to primitive types
   Returns 1 if found, 0 if not a primitive */
static int try_parse_unary_prim(const char *name, UnaryPrimType *out) {
    if (strcmp(name, "add1") == 0) { *out = PRIM_ADD1; return 1; }
    if (strcmp(name, "sub1") == 0) { *out = PRIM_SUB1; return 1; }
    if (strcmp(name, "integer->char") == 0) { *out = PRIM_INTEGER_TO_CHAR; return 1; }
    if (strcmp(name, "char->integer") == 0) { *out = PRIM_CHAR_TO_INTEGER; return 1; }
    if (strcmp(name, "zero?") == 0) { *out = PRIM_ZERO_P; return 1; }
    if (strcmp(name, "null?") == 0) { *out = PRIM_NULL_P; return 1; }
    if (strcmp(name, "integer?") == 0) { *out = PRIM_INTEGER_P; return 1; }
    if (strcmp(name, "boolean?") == 0) { *out = PRIM_BOOLEAN_P; return 1; }
    if (strcmp(name, "char?") == 0) { *out = PRIM_CHAR_P; return 1; }
    return 0;
}

static int try_parse_binary_prim(const char *name, BinaryPrimType *out) {
    if (strcmp(name, "+") == 0) { *out = PRIM_PLUS; return 1; }
    if (strcmp(name, "-") == 0) { *out = PRIM_MINUS; return 1; }
    if (strcmp(name, "*") == 0) { *out = PRIM_MULTIPLY; return 1; }
    if (strcmp(name, "=") == 0) { *out = PRIM_EQUALS; return 1; }
    if (strcmp(name, "<") == 0) { *out = PRIM_LESS; return 1; }
    if (strcmp(name, ">") == 0) { *out = PRIM_GREATER; return 1; }
    if (strcmp(name, "<=") == 0) { *out = PRIM_LESS_EQUAL; return 1; }
    if (strcmp(name, ">=") == 0) { *out = PRIM_GREATER_EQUAL; return 1; }
    if (strcmp(name, "char=?") == 0) { *out = PRIM_CHAR_EQUAL; return 1; }
    if (strcmp(name, "char<?") == 0) { *out = PRIM_CHAR_LESS; return 1; }
    return 0;
}

static void advance(void) {
    current_token = next_token();
}

static void expect(TokenType type) {
    if (current_token.type != type) {
        fprintf(stderr, "Error: Unexpected token (expected %d, got %d)\n", type, current_token.type);
        exit(1);
    }
    advance();
}

static Expr* parse_expr(void);

static Expr* parse_primary(void) {
    if (current_token.type == TOK_NUMBER) {
        int val = current_token.value;
        advance();
        return expr_fixnum(val);
    } else if (current_token.type == TOK_TRUE) {
        advance();
        return expr_boolean(1);
    } else if (current_token.type == TOK_FALSE) {
        advance();
        return expr_boolean(0);
    } else if (current_token.type == TOK_CHAR) {
        char ch = current_token.char_value;
        advance();
        return expr_character(ch);
    } else if (current_token.type == TOK_EMPTY_LIST) {
        advance();
        return expr_empty_list();
    } else if (current_token.type == TOK_IDENTIFIER) {
        /* Variable reference */
        char *name = current_token.identifier;
        advance();
        return expr_variable(name);
    } else if (current_token.type == TOK_LPAREN) {
        advance();
        
        /* Check for binary operators as function calls: +, -, *, =, <, > */
        if (current_token.type == TOK_PLUS) {
            advance();
            Expr* arg1 = parse_expr();
            Expr* arg2 = parse_expr();
            expect(TOK_RPAREN);
            return expr_binary_prim(PRIM_PLUS, arg1, arg2);
        } else if (current_token.type == TOK_MINUS) {
            advance();
            Expr* arg1 = parse_expr();
            Expr* arg2 = parse_expr();
            expect(TOK_RPAREN);
            return expr_binary_prim(PRIM_MINUS, arg1, arg2);
        } else if (current_token.type == TOK_STAR) {
            advance();
            Expr* arg1 = parse_expr();
            Expr* arg2 = parse_expr();
            expect(TOK_RPAREN);
            return expr_binary_prim(PRIM_MULTIPLY, arg1, arg2);
        } else if (current_token.type == TOK_EQUALS) {
            advance();
            Expr* arg1 = parse_expr();
            Expr* arg2 = parse_expr();
            expect(TOK_RPAREN);
            return expr_binary_prim(PRIM_EQUALS, arg1, arg2);
        } else if (current_token.type == TOK_LESS) {
            advance();
            Expr* arg1 = parse_expr();
            Expr* arg2 = parse_expr();
            expect(TOK_RPAREN);
            return expr_binary_prim(PRIM_LESS, arg1, arg2);
        } else if (current_token.type == TOK_GREATER) {
            advance();
            Expr* arg1 = parse_expr();
            Expr* arg2 = parse_expr();
            expect(TOK_RPAREN);
            return expr_binary_prim(PRIM_GREATER, arg1, arg2);
        }
        
        /* Check if it's a special form or procedure call (identifier-based) */
        if (current_token.type == TOK_IDENTIFIER) {
            char *name = current_token.identifier;
            /* Make a copy of name since current_token will be overwritten */
            char *name_copy = malloc(strlen(name) + 1);
            strcpy(name_copy, name);
            advance();
            
            /* Check for let expression: (let (var value) body) */
            if (strcmp(name_copy, "let") == 0) {
                expect(TOK_LPAREN);
                if (current_token.type != TOK_IDENTIFIER) {
                    fprintf(stderr, "Error: Expected variable name in let binding\n");
                    exit(1);
                }
                const char *var = current_token.identifier;  /* Don't copy, expr_let will copy it */
                advance();
                Expr *init = parse_expr();
                expect(TOK_RPAREN);
                Expr *body = parse_expr();
                expect(TOK_RPAREN);
                return expr_let(var, init, body);
            }
            
            /* Check for if expression: (if test consequent alternate) */
            if (strcmp(name_copy, "if") == 0) {
                Expr *test = parse_expr();
                Expr *consequent = parse_expr();
                Expr *alternate = parse_expr();
                expect(TOK_RPAREN);
                return expr_if(test, consequent, alternate);
            }
            
            /* Check for cons: (cons car cdr) */
            if (strcmp(name_copy, "cons") == 0) {
                Expr *car_expr = parse_expr();
                Expr *cdr_expr = parse_expr();
                expect(TOK_RPAREN);
                return expr_cons(car_expr, cdr_expr);
            }
            
            /* Check for car: (car pair) */
            if (strcmp(name_copy, "car") == 0) {
                Expr *pair = parse_expr();
                expect(TOK_RPAREN);
                return expr_car(pair);
            }
            
            /* Check for cdr: (cdr pair) */
            if (strcmp(name_copy, "cdr") == 0) {
                Expr *pair = parse_expr();
                expect(TOK_RPAREN);
                return expr_cdr(pair);
            }
            
            /* Try to parse as unary primitive */
            UnaryPrimType unary;
            if (try_parse_unary_prim(name_copy, &unary)) {
                Expr* arg = parse_expr();
                expect(TOK_RPAREN);
                return expr_unary_prim(unary, arg);
            }
            
            /* Try to parse as binary primitive */
            BinaryPrimType binary;
            if (try_parse_binary_prim(name_copy, &binary)) {
                Expr* arg1 = parse_expr();
                Expr* arg2 = parse_expr();
                expect(TOK_RPAREN);
                return expr_binary_prim(binary, arg1, arg2);
            }
            
            /* Unknown function */
            fprintf(stderr, "Error: Unknown primitive: %s\n", name_copy);
            exit(1);
        } else {
            /* Just a grouped expression */
            Expr* val = parse_expr();
            expect(TOK_RPAREN);
            return val;
        }
    } else {
        fprintf(stderr, "Error: Unexpected token in primary expression (type %d)\n", current_token.type);
        exit(1);
    }
}

/* Parse term: handles * operator (higher precedence) */
static Expr* parse_term(void) {
    Expr* left = parse_primary();
    
    while (current_token.type == TOK_STAR) {
        advance();
        Expr* right = parse_primary();
        /* Use binary primitive for multiplication */
        left = expr_binary_prim(PRIM_MULTIPLY, left, right);
    }
    
    return left;
}

/* Parse expression: handles +/- operators (lower precedence) */
static Expr* parse_expr(void) {
    Expr* left = parse_term();
    
    while (current_token.type == TOK_PLUS || current_token.type == TOK_MINUS) {
        TokenType op = current_token.type;
        advance();
        Expr* right = parse_term();
        if (op == TOK_PLUS) {
            left = expr_binary_prim(PRIM_PLUS, left, right);
        } else {
            left = expr_binary_prim(PRIM_MINUS, left, right);
        }
    }
    
    return left;
}

Expr* parse_program(const char *input) {
    lexer_init(input);
    advance();
    
    /* Optional: support "return expr;" syntax for backwards compatibility */
    if (current_token.type == TOK_RETURN) {
        advance();
    }
    
    Expr* result = parse_expr();
    
    /* Optional: expect semicolon if present */
    if (current_token.type == TOK_SEMICOLON) {
        advance();
    }
    
    if (current_token.type != TOK_EOF) {
        fprintf(stderr, "Error: Expected end of input\n");
        exit(1);
    }
    return result;
}

