#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

// --- small helpers ---

static char peek(Lexer *lx) {
    return lx->src[lx->pos];
}

static char peek_next(Lexer *lx) {
    char c = lx->src[lx->pos];
    if (c == '\0') return '\0';
    return lx->src[lx->pos + 1];
}

static char advance(Lexer *lx) {
    char c = lx->src[lx->pos];
    if (c == '\0') return '\0';

    lx->pos++;
    if (c == '\n') {
        lx->line++;
        lx->col = 1;
    } else {
        lx->col++;
    }
    return c;
}

static void skip_whitespace_and_comments(Lexer *lx) {
    for (;;) {
        char c = peek(lx);

        // whitespace
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance(lx);
            continue;
        }

        // comment style 1: // ...
        if (c == '/' && peek_next(lx) == '/') {
            while (peek(lx) != '\0' && peek(lx) != '\n') advance(lx);
            continue;
        }

        // comment style 2: # ...
        if (c == '#') {
            while (peek(lx) != '\0' && peek(lx) != '\n') advance(lx);
            continue;
        }

        break;
    }
}

static Token make_token(TokenType type, const char *lexeme, int line, int col) {
    Token t;
    t.type = type;
    t.line = line;
    t.col = col;
    // safe copy into fixed buffer
    if (lexeme) {
        strncpy(t.lexeme, lexeme, sizeof(t.lexeme) - 1);
        t.lexeme[sizeof(t.lexeme) - 1] = '\0';
    } else {
        t.lexeme[0] = '\0';
    }
    return t;
}

void lexer_init(Lexer *lx, const char *source) {
    lx->src = source;
    lx->pos = 0;
    lx->line = 1;
    lx->col = 1;
}

static Token lex_identifier_or_keyword(Lexer *lx, int start_line, int start_col) {
    char buf[256];
    size_t n = 0;

    while (isalnum((unsigned char)peek(lx)) || peek(lx) == '_' || peek(lx) == '-') {
        if (n + 1 < sizeof(buf)) buf[n++] = advance(lx);
        else advance(lx); // keep scanning even if buffer full
    }
    buf[n] = '\0';

    // keywords
    if (strcmp(buf, "true") == 0)  return make_token(TOK_TRUE, buf, start_line, start_col);
    if (strcmp(buf, "false") == 0) return make_token(TOK_FALSE, buf, start_line, start_col);

    return make_token(TOK_ID, buf, start_line, start_col);
}

static Token lex_number(Lexer *lx, int start_line, int start_col) {
    char buf[256];
    size_t n = 0;

    while (isdigit((unsigned char)peek(lx))) {
        if (n + 1 < sizeof(buf)) buf[n++] = advance(lx);
        else advance(lx);
    }
    buf[n] = '\0';
    return make_token(TOK_NUMBER, buf, start_line, start_col);
}

static Token lex_string(Lexer *lx, int start_line, int start_col) {
    // consume opening quote
    advance(lx);

    char buf[256];
    size_t n = 0;

    while (peek(lx) != '\0' && peek(lx) != '"') {
        char c = advance(lx);

        // handle very basic escapes: \" and \\ and \n and \t
        if (c == '\\') {
            char e = peek(lx);
            if (e == 'n') { advance(lx); c = '\n'; }
            else if (e == 't') { advance(lx); c = '\t'; }
            else if (e == '"' ) { advance(lx); c = '"'; }
            else if (e == '\\') { advance(lx); c = '\\'; }
            // else: unknown escape, keep char as-is
        }

        if (n + 1 < sizeof(buf)) buf[n++] = c;
    }

    // if closing quote exists, consume it
    if (peek(lx) == '"') advance(lx);

    buf[n] = '\0';
    return make_token(TOK_STRING, buf, start_line, start_col);
}

Token lexer_next(Lexer *lx) {
    skip_whitespace_and_comments(lx);

    int start_line = lx->line;
    int start_col  = lx->col;

    char c = peek(lx);
    if (c == '\0') {
        return make_token(TOK_EOF, "", start_line, start_col);
    }

    // single-character tokens
    if (c == '{') { advance(lx); return make_token(TOK_LBRACE, "{", start_line, start_col); }
    if (c == '}') { advance(lx); return make_token(TOK_RBRACE, "}", start_line, start_col); }
    if (c == '[') { advance(lx); return make_token(TOK_LBRACKET, "[", start_line, start_col); }
    if (c == ']') { advance(lx); return make_token(TOK_RBRACKET, "]", start_line, start_col); }
    if (c == '=') { advance(lx); return make_token(TOK_EQUAL, "=", start_line, start_col); }
    if (c == ',') { advance(lx); return make_token(TOK_COMMA, ",", start_line, start_col); }
    if (c == ';') { advance(lx); return make_token(TOK_SEMI, ";", start_line, start_col); }

    // string
    if (c == '"') {
        return lex_string(lx, start_line, start_col);
    }

    // identifier / keyword
    if (isalpha((unsigned char)c) || c == '_') {
        return lex_identifier_or_keyword(lx, start_line, start_col);
    }

    // number
    if (isdigit((unsigned char)c)) {
        return lex_number(lx, start_line, start_col);
    }

    // unknown character(s): group consecutive invalid characters into ONE error token
    {
        char bad[128];
        size_t n = 0;

        while (1) {
            char ch = peek(lx);
            if (ch == '\0') break;

            // stop grouping at whitespace (skip_whitespace_and_comments will handle it)
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') break;

            // stop grouping at comment start
            if (ch == '#') break;
            if (ch == '/' && peek_next(lx) == '/') break;

            // stop grouping at valid single-character tokens
            if (ch == '{' || ch == '}' || ch == '[' || ch == ']' ||
                ch == '=' || ch == ',' || ch == ';') break;

            // stop grouping at valid identifier/number/string starts
            if (isalpha((unsigned char)ch) || ch == '_' ||
                isdigit((unsigned char)ch) || ch == '"') break;

            // invalid -> consume
            if (n + 1 < sizeof(bad)) bad[n++] = advance(lx);
            else advance(lx);
        }

        bad[n] = '\0';

        char msg[256];
        snprintf(msg, sizeof(msg), "Unexpected characters \"%s\"", bad);
        return make_token(TOK_ERROR, msg, start_line, start_col);
    }
}

const char* token_type_name(TokenType t) {
    switch (t) {
        case TOK_ERROR: return "ERROR";
        case TOK_EOF: return "EOF";
        case TOK_ID: return "ID";
        case TOK_NUMBER: return "NUMBER";
        case TOK_STRING: return "STRING";
        case TOK_TRUE: return "TRUE";
        case TOK_FALSE: return "FALSE";
        case TOK_LBRACE: return "{";
        case TOK_RBRACE: return "}";
        case TOK_LBRACKET: return "[";
        case TOK_RBRACKET: return "]";
        case TOK_EQUAL: return "=";
        case TOK_COMMA: return ",";
        case TOK_SEMI: return ";";
        default: return "UNKNOWN";
    }
}