#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static Lexer lexer;

void lexer_init(const char *input) {
    lexer.input = input;
    lexer.pos = 0;
    lexer.len = strlen(input);
}

static void skip_whitespace(void) {
    while (lexer.pos < lexer.len && isspace(lexer.input[lexer.pos])) {
        lexer.pos++;
    }
}

Token next_token(void) {
    skip_whitespace();
    
    if (lexer.pos >= lexer.len) {
        return (Token){TOK_EOF, 0, 0, NULL};
    }
    
    char c = lexer.input[lexer.pos];
    
    /* Handle #t, #f, #\char, () */
    if (c == '#') {
        lexer.pos++;
        if (lexer.pos >= lexer.len) {
            fprintf(stderr, "Error: Incomplete immediate constant\n");
            exit(1);
        }
        char next = lexer.input[lexer.pos];
        if (next == 't') {
            lexer.pos++;
            if (lexer.pos < lexer.len && (isalnum(lexer.input[lexer.pos]) || lexer.input[lexer.pos] == '_')) {
                fprintf(stderr, "Error: Invalid immediate constant\n");
                exit(1);
            }
            return (Token){TOK_TRUE, 0, 0, NULL};
        } else if (next == 'f') {
            lexer.pos++;
            if (lexer.pos < lexer.len && (isalnum(lexer.input[lexer.pos]) || lexer.input[lexer.pos] == '_')) {
                fprintf(stderr, "Error: Invalid immediate constant\n");
                exit(1);
            }
            return (Token){TOK_FALSE, 0, 0, NULL};
        } else if (next == '\\') {
            lexer.pos++;
            if (lexer.pos >= lexer.len) {
                fprintf(stderr, "Error: Incomplete character constant\n");
                exit(1);
            }
            
            /* Handle named characters like #\space, #\newline, etc. */
            if (isalpha(lexer.input[lexer.pos])) {
                const char *start = &lexer.input[lexer.pos];
                while (lexer.pos < lexer.len && isalpha(lexer.input[lexer.pos])) {
                    lexer.pos++;
                }
                int len = &lexer.input[lexer.pos] - start;
                
                if (len == 1) {
                    /* Single character like #\A */
                    return (Token){TOK_CHAR, 0, start[0], NULL};
                } else if (len == 5 && strncmp(start, "space", 5) == 0) {
                    return (Token){TOK_CHAR, 0, ' ', NULL};
                } else if (len == 7 && strncmp(start, "newline", 7) == 0) {
                    return (Token){TOK_CHAR, 0, '\n', NULL};
                } else if (len == 3 && strncmp(start, "tab", 3) == 0) {
                    return (Token){TOK_CHAR, 0, '\t', NULL};
                } else {
                    fprintf(stderr, "Error: Unknown named character\n");
                    exit(1);
                }
            } else {
                char ch = lexer.input[lexer.pos];
                lexer.pos++;
                return (Token){TOK_CHAR, 0, ch, NULL};
            }
        } else {
            fprintf(stderr, "Error: Unknown immediate constant\n");
            exit(1);
        }
    }
    
    /* Handle () */
    if (c == '(' && lexer.pos + 1 < lexer.len && lexer.input[lexer.pos + 1] == ')') {
        lexer.pos += 2;
        return (Token){TOK_EMPTY_LIST, 0, 0, NULL};
    }
    
    if (isdigit(c)) {
        int num = 0;
        while (lexer.pos < lexer.len && isdigit(lexer.input[lexer.pos])) {
            num = num * 10 + (lexer.input[lexer.pos] - '0');
            lexer.pos++;
        }
        return (Token){TOK_NUMBER, num, 0, NULL};
    }
    
    if (isalpha(c) || c == '_') {
        const char *start = &lexer.input[lexer.pos];
        while (lexer.pos < lexer.len && (isalnum(lexer.input[lexer.pos]) || lexer.input[lexer.pos] == '_' || lexer.input[lexer.pos] == '?' || lexer.input[lexer.pos] == '-' || lexer.input[lexer.pos] == '>')) {
            lexer.pos++;
        }
        int len = &lexer.input[lexer.pos] - start;
        
        if (len == 6 && strncmp(start, "return", 6) == 0) {
            return (Token){TOK_RETURN, 0, 0, NULL};
        }
        
        /* Allocate memory for identifier */
        char *ident = malloc(len + 1);
        strncpy(ident, start, len);
        ident[len] = '\0';
        return (Token){TOK_IDENTIFIER, 0, 0, ident};
    }
    
    lexer.pos++;
    switch (c) {
        case '+': return (Token){TOK_PLUS, 0, 0, NULL};
        case '-': return (Token){TOK_MINUS, 0, 0, NULL};
        case '*': return (Token){TOK_STAR, 0, 0, NULL};
        case '/': return (Token){TOK_SLASH, 0, 0, NULL};
        case '=': return (Token){TOK_EQUALS, 0, 0, NULL};
        case '<': return (Token){TOK_LESS, 0, 0, NULL};
        case '>': return (Token){TOK_GREATER, 0, 0, NULL};
        case '?': return (Token){TOK_QUESTION, 0, 0, NULL};
        case '(': return (Token){TOK_LPAREN, 0, 0, NULL};
        case ')': return (Token){TOK_RPAREN, 0, 0, NULL};
        case ';': return (Token){TOK_SEMICOLON, 0, 0, NULL};
        default:
            fprintf(stderr, "Error: Unknown character '%c'\n", c);
            exit(1);
    }
}

/* Helper function to get token type name */
static const char *token_type_name(TokenType type) {
    switch (type) {
        case TOK_EOF: return "TOK_EOF";
        case TOK_RETURN: return "TOK_RETURN";
        case TOK_NUMBER: return "TOK_NUMBER";
        case TOK_TRUE: return "TOK_TRUE";
        case TOK_FALSE: return "TOK_FALSE";
        case TOK_CHAR: return "TOK_CHAR";
        case TOK_EMPTY_LIST: return "TOK_EMPTY_LIST";
        case TOK_IDENTIFIER: return "TOK_IDENTIFIER";
        case TOK_PLUS: return "TOK_PLUS";
        case TOK_MINUS: return "TOK_MINUS";
        case TOK_STAR: return "TOK_STAR";
        case TOK_LPAREN: return "TOK_LPAREN";
        case TOK_RPAREN: return "TOK_RPAREN";
        case TOK_SEMICOLON: return "TOK_SEMICOLON";
        case TOK_SLASH: return "TOK_SLASH";
        case TOK_EQUALS: return "TOK_EQUALS";
        case TOK_LESS: return "TOK_LESS";
        case TOK_GREATER: return "TOK_GREATER";
        case TOK_QUESTION: return "TOK_QUESTION";
        default: return "UNKNOWN";
    }
}

/* Dump all tokens from input to file */
void dump_tokens_to_file(const char *input, const char *output_file) {
    FILE *out = fopen(output_file, "w");
    if (!out) {
        fprintf(stderr, "Error: Could not open '%s' for writing\n", output_file);
        return;
    }
    
    lexer_init(input);
    int token_count = 0;
    
    fprintf(out, "# Token Stream\n\n");
    fprintf(out, "Source: %s\n\n", input);
    fprintf(out, "## Tokens\n\n");
    
    while (1) {
        Token tok = next_token();
        token_count++;
        
        fprintf(out, "Token %d: %s", token_count, token_type_name(tok.type));
        
        /* Output additional info based on token type */
        if (tok.type == TOK_NUMBER) {
            fprintf(out, " (value: %d)", tok.value);
        } else if (tok.type == TOK_CHAR) {
            if (tok.char_value == ' ') {
                fprintf(out, " (value: 'space')");
            } else if (tok.char_value == '\n') {
                fprintf(out, " (value: 'newline')");
            } else if (tok.char_value == '\t') {
                fprintf(out, " (value: 'tab')");
            } else if (tok.char_value >= 32 && tok.char_value < 127) {
                fprintf(out, " (value: '%c')", tok.char_value);
            } else {
                fprintf(out, " (value: 0x%02x)", (unsigned char)tok.char_value);
            }
        } else if (tok.type == TOK_IDENTIFIER) {
            fprintf(out, " (name: %s)", tok.identifier);
        }
        
        fprintf(out, "\n");
        
        if (tok.type == TOK_EOF) {
            break;
        }
    }
    
    fprintf(out, "\n## Summary\n\n");
    fprintf(out, "Total tokens: %d\n", token_count);
    
    fclose(out);
}
