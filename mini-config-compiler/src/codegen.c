#include "codegen.h"
#include <stdio.h>

/* ---------- indentation helper ---------- */

static void indent(FILE *out, int level) {
    for (int i = 0; i < level; i++)
        fprintf(out, "  ");
}

/* ---------- forward declarations ---------- */

static void emit_value(FILE *out, const Value *v, int level);
static void emit_object(FILE *out, const Object *obj, int level);
static void emit_array(FILE *out, const Array *arr, int level);

/* ---------- value emitter ---------- */

static void emit_value(FILE *out, const Value *v, int level) {
    switch (v->type) {

        case VAL_NUMBER:
            fprintf(out, "%ld", v->as.number);
            break;

        case VAL_BOOL:
            fprintf(out, v->as.boolean ? "true" : "false");
            break;

        case VAL_STRING:
            fprintf(out, "\"%s\"", v->as.string);
            break;

        case VAL_OBJECT:
            emit_object(out, &v->as.object, level);
            break;

        case VAL_ARRAY:
            emit_array(out, &v->as.array, level);
            break;
    }
}

/* ---------- object emitter ---------- */

static void emit_object(FILE *out, const Object *obj, int level) {
    fprintf(out, "{\n");

    for (size_t i = 0; i < obj->count; i++) {
        indent(out, level + 1);

        fprintf(out, "\"%s\": ", obj->pairs[i].key);

        emit_value(out, obj->pairs[i].value, level + 1);

        if (i + 1 < obj->count)
            fprintf(out, ",");

        fprintf(out, "\n");
    }

    indent(out, level);
    fprintf(out, "}");
}

/* ---------- array emitter ---------- */

static void emit_array(FILE *out, const Array *arr, int level) {
    fprintf(out, "[");

    for (size_t i = 0; i < arr->count; i++) {
        emit_value(out, arr->items[i], level);

        if (i + 1 < arr->count)
            fprintf(out, ", ");
    }

    fprintf(out, "]");
}

/* ---------- public entry ---------- */

void codegen_json(FILE *out, const Program *prog) {
    emit_object(out, &prog->root, 0);
    fprintf(out, "\n");
}