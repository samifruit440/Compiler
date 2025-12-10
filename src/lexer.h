#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOK_EOF,
    TOK_RETURN,
    TOK_NUMBER,
    TOK_TRUE,
    TOK_FALSE,
    TOK_CHAR,
    TOK_EMPTY_LIST,
    TOK_IDENTIFIER,  /* For primitive function names */
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_SEMICOLON,
    TOK_SLASH,       /* For division or / in names */
    TOK_EQUALS,      /* For = predicate */
    TOK_LESS,        /* For < predicate */
    TOK_GREATER,     /* For > predicate */
    TOK_QUESTION     /* For ? in predicate names */
} TokenType;

typedef struct {
    TokenType type;
    int value;
    char char_value;
    char *identifier;  /* For TOK_IDENTIFIER */
} Token;

typedef struct {
    const char *input;
    int pos;
    int len;
} Lexer;

/* Initialize lexer with input string */
void lexer_init(const char *input);

/* Get next token */
Token next_token(void);

/* Dump all tokens to file */
void dump_tokens_to_file(const char *input, const char *output_file);

#endif
