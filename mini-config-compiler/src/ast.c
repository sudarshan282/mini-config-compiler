#include "ast.h"
#include <stdlib.h>
#include <string.h>

// simple strdup (portable)
static char* str_dup(const char *s) {
    size_t n = strlen(s);
    char *out = (char*)malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, s, n + 1);
    return out;
}

/* ---- program ---- */

Program program_make(void) {
    Program p;
    p.root.pairs = NULL;
    p.root.count = 0;
    p.root.capacity = 0;
    return p;
}

static void object_free(Object *obj) {
    for (size_t i = 0; i < obj->count; i++) {
        free(obj->pairs[i].key);
        value_free(obj->pairs[i].value);
    }
    free(obj->pairs);
    obj->pairs = NULL;
    obj->count = 0;
    obj->capacity = 0;
}

static void array_free(Array *arr) {
    for (size_t i = 0; i < arr->count; i++) {
        value_free(arr->items[i]);
    }
    free(arr->items);
    arr->items = NULL;
    arr->count = 0;
    arr->capacity = 0;
}

void program_free(Program *p) {
    object_free(&p->root);
}

/* ---- value constructors ---- */

Value* value_make_number(long n) {
    Value *v = (Value*)calloc(1, sizeof(Value));
    if (!v) return NULL;
    v->type = VAL_NUMBER;
    v->as.number = n;
    return v;
}

Value* value_make_bool(int b) {
    Value *v = (Value*)calloc(1, sizeof(Value));
    if (!v) return NULL;
    v->type = VAL_BOOL;
    v->as.boolean = b ? 1 : 0;
    return v;
}

Value* value_make_string(const char *s) {
    Value *v = (Value*)calloc(1, sizeof(Value));
    if (!v) return NULL;
    v->type = VAL_STRING;
    v->as.string = str_dup(s ? s : "");
    return v;
}

Value* value_make_array(void) {
    Value *v = (Value*)calloc(1, sizeof(Value));
    if (!v) return NULL;
    v->type = VAL_ARRAY;
    v->as.array.items = NULL;
    v->as.array.count = 0;
    v->as.array.capacity = 0;
    return v;
}

Value* value_make_object(void) {
    Value *v = (Value*)calloc(1, sizeof(Value));
    if (!v) return NULL;
    v->type = VAL_OBJECT;
    v->as.object.pairs = NULL;
    v->as.object.count = 0;
    v->as.object.capacity = 0;
    return v;
}

void value_free(Value *v) {
    if (!v) return;

    switch (v->type) {
        case VAL_STRING:
            free(v->as.string);
            break;
        case VAL_ARRAY:
            array_free(&v->as.array);
            break;
        case VAL_OBJECT:
            object_free(&v->as.object);
            break;
        case VAL_NUMBER:
        case VAL_BOOL:
        default:
            break;
    }

    free(v);
}

/* ---- add helpers ---- */

void object_add_pair(Object *obj, const char *key, Value *value, int line, int col) {
    if (obj->count + 1 > obj->capacity) {
        size_t new_cap = (obj->capacity == 0) ? 8 : obj->capacity * 2;
        Pair *new_pairs = (Pair*)realloc(obj->pairs, new_cap * sizeof(Pair));
        if (!new_pairs) return;
        obj->pairs = new_pairs;
        obj->capacity = new_cap;
    }

    obj->pairs[obj->count].key = str_dup(key);
    obj->pairs[obj->count].value = value;
    obj->pairs[obj->count].line = line;
    obj->pairs[obj->count].col  = col;
    obj->count++;
}

void array_add_item(Array *arr, Value *value) {
    if (arr->count + 1 > arr->capacity) {
        size_t new_cap = (arr->capacity == 0) ? 8 : arr->capacity * 2;
        Value **new_items = (Value**)realloc(arr->items, new_cap * sizeof(Value*));
        if (!new_items) return;
        arr->items = new_items;
        arr->capacity = new_cap;
    }
    arr->items[arr->count] = value;
    arr->count++;
}