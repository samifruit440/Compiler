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
    EXPR_VARIABLE,
    EXPR_LET,
    EXPR_IF,
    EXPR_CONS,
    EXPR_CAR,
    EXPR_CDR,
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

typedef struct {
    char *name;  /* Variable name */
} VariableExpr;

typedef struct {
    char *name;        /* Variable name being bound */
    Expr *init;        /* Initial value expression */
    Expr *body;        /* Body expression with variable in scope */
} LetExpr;

typedef struct {
    Expr *test;        /* Test expression */
    Expr *consequent;  /* Then branch */
    Expr *alternate;   /* Else branch */
} IfExpr;

typedef struct {
    Expr *car_expr;    /* car (first element) */
    Expr *cdr_expr;    /* cdr (rest/second element) */
} ConsExpr;

typedef struct {
    Expr *pair;        /* Pair to extract from */
} CarExpr;

typedef struct {
    Expr *pair;        /* Pair to extract from */
} CdrExpr;

typedef struct Expr {
    ExprType type;
    union {
        FixnumExpr fixnum;
        BooleanExpr boolean;
        CharacterExpr character;
        UnaryPrimExpr unary_prim;
        BinaryPrimExpr binary_prim;
        VariableExpr variable;
        LetExpr let_expr;
        IfExpr if_expr;
        ConsExpr cons;
        CarExpr car;
        CdrExpr cdr;
    } data;
} Expr;

/* Constructors */
Expr* expr_fixnum(int value);
Expr* expr_boolean(int value);
Expr* expr_character(char value);
Expr* expr_empty_list(void);
Expr* expr_unary_prim(UnaryPrimType op, Expr *operand);
Expr* expr_binary_prim(BinaryPrimType op, Expr *operand1, Expr *operand2);
Expr* expr_variable(const char *name);
Expr* expr_let(const char *name, Expr *init, Expr *body);
Expr* expr_if(Expr *test, Expr *consequent, Expr *alternate);
Expr* expr_cons(Expr *car_expr, Expr *cdr_expr);
Expr* expr_car(Expr *pair);
Expr* expr_cdr(Expr *pair);

/* Memory management */
void expr_free(Expr *expr);

#endif
