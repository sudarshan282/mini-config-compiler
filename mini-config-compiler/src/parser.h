#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

// Parser = lexer + current token + error flag
typedef struct {
    Lexer lx;
    Token current;
    int had_error;
} Parser;

// Initialize parser using input source text
void parser_init(Parser *ps, const char *source);

// Parse entire program into AST Program
Program parser_parse_program(Parser *ps);

#endif