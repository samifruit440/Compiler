#ifndef AST_H
#define AST_H

/* Abstract Syntax Tree node types */

typedef enum {
    EXPR_FIXNUM,
    EXPR_BOOLEAN,
    EXPR_CHARACTER,
    EXPR_EMPTY_LIST,
    EXPR_UNARY_PRIM,
    EXPR_BINARY_PRIM,
} ExprType;

typedef enum {
    PRIM_ADD1,
    PRIM_SUB1,
    PRIM_INTEGER_TO_CHAR,
    PRIM_CHAR_TO_INTEGER,
    PRIM_ZERO_P,
    PRIM_NULL_P,
    PRIM_INTEGER_P,
    PRIM_BOOLEAN_P,
    PRIM_CHAR_P,
} UnaryPrimType;

typedef enum {
    PRIM_PLUS,
    PRIM_MINUS,
    PRIM_MULTIPLY,
    PRIM_EQUALS,
    PRIM_LESS,
    PRIM_GREATER,
    PRIM_LESS_EQUAL,
    PRIM_GREATER_EQUAL,
    PRIM_CHAR_EQUAL,
    PRIM_CHAR_LESS,
} BinaryPrimType;

/* Forward declaration */
typedef struct Expr Expr;

typedef struct {
    int value;
} FixnumExpr;

typedef struct {
    int value;
} BooleanExpr;

typedef struct {
    char value;
} CharacterExpr;

typedef struct {
    UnaryPrimType op;
    Expr *operand;
} UnaryPrimExpr;

typedef struct {
    BinaryPrimType op;
    Expr *operand1;
    Expr *operand2;
} BinaryPrimExpr;

typedef struct Expr {
    ExprType type;
    union {
        FixnumExpr fixnum;
        BooleanExpr boolean;
        CharacterExpr character;
        UnaryPrimExpr unary_prim;
        BinaryPrimExpr binary_prim;
    } data;
} Expr;

/* Constructors */
Expr* expr_fixnum(int value);
Expr* expr_boolean(int value);
Expr* expr_character(char value);
Expr* expr_empty_list(void);
Expr* expr_unary_prim(UnaryPrimType op, Expr *operand);
Expr* expr_binary_prim(BinaryPrimType op, Expr *operand1, Expr *operand2);

/* Memory management */
void expr_free(Expr *expr);

#endif
