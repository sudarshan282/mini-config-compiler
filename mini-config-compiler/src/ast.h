#ifndef AST_H
#define AST_H

#include <stddef.h>

// Value types for config language
typedef enum {
    VAL_NUMBER,
    VAL_STRING,
    VAL_BOOL,
    VAL_ARRAY,
    VAL_OBJECT
} ValueType;

struct Value; // forward declaration

// A key-value pair inside an object:  key = value  OR key { ... }
typedef struct {
    char *key;
    struct Value *value;

    // Position of the key in source file (from lexer token)
    int line;
    int col;
} Pair;

// Object = list of pairs
typedef struct {
    Pair *pairs;
    size_t count;
    size_t capacity;
} Object;

// Array = list of values
typedef struct {
    struct Value **items;
    size_t count;
    size_t capacity;
} Array;

// Value node
typedef struct Value {
    ValueType type;
    union {
        long number;   // integers only (for now)
        char *string;
        int boolean;   // 0/1
        Array array;
        Object object;
    } as;
} Value;

// Root program: just an object of top-level pairs
typedef struct {
    Object root;
} Program;

/* ---- constructors (make nodes) ---- */

Program program_make(void);
void program_free(Program *p);

Value* value_make_number(long n);
Value* value_make_bool(int b);
Value* value_make_string(const char *s);
Value* value_make_array(void);
Value* value_make_object(void);

void value_free(Value *v);

/* ---- helpers to add items ---- */

void object_add_pair(Object *obj, const char *key, Value *value, int line, int col);
void array_add_item(Array *arr, Value *value);

#endif