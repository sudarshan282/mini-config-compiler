#ifndef LEXER_H
#define LEXER_H

#include <stddef.h> // size_t

// All token kinds our language supports (for now).
typedef enum {
    TOK_EOF = 0,
    TOK_ID,
    TOK_ERROR,
    TOK_NUMBER,
    TOK_STRING,
    TOK_TRUE,
    TOK_FALSE,

    TOK_LBRACE,   // {
    TOK_RBRACE,   // }
    TOK_LBRACKET, // [
    TOK_RBRACKET, // ]
    TOK_EQUAL,    // =
    TOK_COMMA,    // ,
    TOK_SEMI      // ;  (optional)
} TokenType;

// Token = type + its text + position info
typedef struct {
    TokenType type;
    char lexeme[256];  // token text (kept small for simplicity)
    int line;
    int col;
} Token;

// Lexer holds the input and current position
typedef struct {
    const char *src;   // whole input text
    size_t pos;        // current index
    int line;          // current line number (1-based)
    int col;           // current column number (1-based)
} Lexer;

// Initialize lexer with input text
void lexer_init(Lexer *lx, const char *source);

// Get next token from input
Token lexer_next(Lexer *lx);

// Token type to readable string (helpful for debugging/errors)
const char* token_type_name(TokenType t);

#endif