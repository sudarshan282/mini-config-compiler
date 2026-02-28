#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
  Parser rules (simplified):
    program  -> stmt* EOF
    stmt     -> ID '=' value [';']
            |  ID '{' stmt* '}' [';']
    value    -> NUMBER | STRING | TRUE | FALSE | array
    array    -> '[' (value (',' value)*)? ']'

  Lex errors:
    The lexer can return TOK_ERROR tokens.
    We do a scan-pass inside parser_parse_program() to print ALL lex errors
    and abort before parsing (prevents cascaded parse errors).
*/

/* ---- forward declarations (fix C "use before define") ---- */
static void advance(Parser *ps);
static int  check(Parser *ps, TokenType t);
static int  match(Parser *ps, TokenType t);
static void error_at_current(Parser *ps, const char *msg);
static void consume(Parser *ps, TokenType t, const char *msg);

static void   parse_stmt(Parser *ps, Object *into);
static Value* parse_value(Parser *ps);
static Value* parse_array(Parser *ps);
/* kept for possible future expansion */
static Value* parse_object_block(Parser *ps);

/* ---------- small helpers ---------- */

static void advance(Parser *ps) {
    ps->current = lexer_next(&ps->lx);
}

static int check(Parser *ps, TokenType t) {
    return ps->current.type == t;
}

static int match(Parser *ps, TokenType t) {
    if (check(ps, t)) {
        advance(ps);
        return 1;
    }
    return 0;
}

static void error_at_current(Parser *ps, const char *msg) {
    fprintf(stderr, "Parse error at %d:%d: %s (got %s \"%s\")\n",
            ps->current.line, ps->current.col,
            msg, token_type_name(ps->current.type), ps->current.lexeme);
    ps->had_error = 1;
}

// Consume a token of expected type, else set error and keep going.
// If we see a TOK_ERROR token, stop with a lex error message.
static void consume(Parser *ps, TokenType t, const char *msg) {
    if (ps->had_error) return;

    if (check(ps, TOK_ERROR)) {
        fprintf(stderr, "Lex error at %d:%d: %s\n",
                ps->current.line, ps->current.col, ps->current.lexeme);
        ps->had_error = 1;
        return;
    }

    if (check(ps, t)) {
        advance(ps);
        return;
    }

    error_at_current(ps, msg);
}

/* ---------- public API ---------- */

void parser_init(Parser *ps, const char *source) {
    lexer_init(&ps->lx, source);
    ps->had_error = 0;
    ps->current = lexer_next(&ps->lx); // load first token
}

Program parser_parse_program(Parser *ps) {
    Program prog = program_make();

    /* Pass 0: scan for lexical errors only, group-free and cascade-free.
       We copy the parser state so we do not consume the real input. */
    {
        Parser scan = *ps; // copy lexer position + current token
        int any_lex_error = 0;

        while (scan.current.type != TOK_EOF) {
            if (scan.current.type == TOK_ERROR) {
                fprintf(stderr, "Lex error at %d:%d: %s\n",
                        scan.current.line, scan.current.col, scan.current.lexeme);
                any_lex_error = 1;
            }
            advance(&scan);
        }

        if (any_lex_error) {
            ps->had_error = 1;
            return prog; // abort before parsing
        }
    }

    // Real parsing: program -> stmt* EOF
    while (!check(ps, TOK_EOF) && !ps->had_error) {
        parse_stmt(ps, &prog.root);
        match(ps, TOK_SEMI); // optional semicolon between statements
    }

    return prog;
}

/* ---------- grammar implementation ---------- */

// stmt -> ID '=' value
//      |  ID '{' stmt* '}'
static void parse_stmt(Parser *ps, Object *into) {
    if (ps->had_error) return;

    if (check(ps, TOK_ERROR)) {
        fprintf(stderr, "Lex error at %d:%d: %s\n",
                ps->current.line, ps->current.col, ps->current.lexeme);
        ps->had_error = 1;
        return;
    }

    if (!check(ps, TOK_ID)) {
        error_at_current(ps, "Expected identifier at start of statement");
        advance(ps); // simple recovery
        return;
    }

    // capture key name + position (where ID starts)
    char key[256];
    int key_line = ps->current.line;
    int key_col  = ps->current.col;

    snprintf(key, sizeof(key), "%s", ps->current.lexeme);
    advance(ps); // consume ID

    if (match(ps, TOK_EQUAL)) {
        // assignment
        Value *v = parse_value(ps);
        if (!v) {
            // parse_value already printed a useful error
            ps->had_error = 1;
            return;
        }
        object_add_pair(into, key, v, key_line, key_col);
        return;
    }

    if (match(ps, TOK_LBRACE)) {
        // object block
        Value *obj = value_make_object();
        if (!obj) {
            ps->had_error = 1;
            return;
        }

        // parse stmt* until }
        while (!check(ps, TOK_RBRACE) && !check(ps, TOK_EOF) && !ps->had_error) {
            parse_stmt(ps, &obj->as.object);
            match(ps, TOK_SEMI);
        }

        consume(ps, TOK_RBRACE, "Expected '}' to close block");
        if (ps->had_error) {
            value_free(obj);
            return;
        }

        object_add_pair(into, key, obj, key_line, key_col);
        return;
    }

    error_at_current(ps, "Expected '=' or '{' after identifier");
}

// value -> NUMBER | STRING | TRUE | FALSE | array
static Value* parse_value(Parser *ps) {
    if (ps->had_error) return NULL;

    if (check(ps, TOK_ERROR)) {
        fprintf(stderr, "Lex error at %d:%d: %s\n",
                ps->current.line, ps->current.col, ps->current.lexeme);
        ps->had_error = 1;
        return NULL;
    }

    if (check(ps, TOK_NUMBER)) {
        long n = strtol(ps->current.lexeme, NULL, 10);
        advance(ps);
        return value_make_number(n);
    }

    if (check(ps, TOK_STRING)) {
        Value *v = value_make_string(ps->current.lexeme);
        advance(ps);
        return v;
    }

    if (check(ps, TOK_TRUE)) {
        advance(ps);
        return value_make_bool(1);
    }

    if (check(ps, TOK_FALSE)) {
        advance(ps);
        return value_make_bool(0);
    }

    if (check(ps, TOK_LBRACKET)) {
        return parse_array(ps);
    }

    error_at_current(ps, "Expected a value (number, string, bool, or array)");
    return NULL;
}

// array -> '[' (value (',' value)*)? ']'
static Value* parse_array(Parser *ps) {
    consume(ps, TOK_LBRACKET, "Expected '[' to start array");
    if (ps->had_error) return NULL;

    Value *arrv = value_make_array();
    if (!arrv) {
        ps->had_error = 1;
        return NULL;
    }

    // empty array?
    if (match(ps, TOK_RBRACKET)) {
        return arrv;
    }

    // first value
    Value *first = parse_value(ps);
    if (!first) { value_free(arrv); return NULL; }
    array_add_item(&arrv->as.array, first);

    // more values
    while (match(ps, TOK_COMMA)) {
        Value *next = parse_value(ps);
        if (!next) { value_free(arrv); return NULL; }
        array_add_item(&arrv->as.array, next);
    }

    consume(ps, TOK_RBRACKET, "Expected ']' to close array");
    if (ps->had_error) { value_free(arrv); return NULL; }

    return arrv;
}

/* Not currently called (kept for future extension).
   Parses the inside of a block after '{' has already been consumed. */
static Value* parse_object_block(Parser *ps) {
    Value *obj = value_make_object();
    if (!obj) { ps->had_error = 1; return NULL; }

    while (!check(ps, TOK_RBRACE) && !check(ps, TOK_EOF) && !ps->had_error) {
        parse_stmt(ps, &obj->as.object);
        match(ps, TOK_SEMI);
    }

    consume(ps, TOK_RBRACE, "Expected '}' to close block");
    if (ps->had_error) { value_free(obj); return NULL; }

    return obj;
}