#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

/* Parse a complete program and return an AST expression */
Expr* parse_program(const char *input);

#endif
