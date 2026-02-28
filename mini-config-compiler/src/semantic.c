#include "semantic.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Check duplicates inside ONE object
static int check_object_duplicates(const Object *obj, const char *path);

// Walk any value (only objects/arrays contain nested stuff)
static int check_value(const Value *v, const char *path);

// Build "path.key" strings for nicer error messages
static void make_child_path(char *out, size_t out_sz, const char *parent, const char *key) {
    if (!parent || parent[0] == '\0') {
        // top level
        snprintf(out, out_sz, "%s", key);
    } else {
        snprintf(out, out_sz, "%s.%s", parent, key);
    }
}

int semantic_check_program(const Program *p) {
    // top-level object path = ""
    return check_object_duplicates(&p->root, "");
}

static int check_object_duplicates(const Object *obj, const char *path) {
    int ok = 1;

    // marks indices already treated as duplicates
    char *skip = (char*)calloc(obj->count, 1);

    for (size_t i = 0; i < obj->count; i++) {
        if (skip && skip[i]) continue;

        const char *key = obj->pairs[i].key;
        int found_dup = 0;

        // search later duplicates
        for (size_t j = i + 1; j < obj->count; j++) {
            if (skip && skip[j]) continue;

            if (strcmp(key, obj->pairs[j].key) == 0) {
                if (skip) skip[j] = 1;
                found_dup = 1;
            }
        }

        if (found_dup) {
            char full[512];
            make_child_path(full, sizeof(full), path, key);

            const Pair *first = &obj->pairs[i];

            fprintf(stderr,
                "Semantic error: duplicate key '%s'\n"
                "  first: %d:%d\n",
                full,
                first->line, first->col
            );

            // print all duplicates
            for (size_t j = i + 1; j < obj->count; j++) {
                if (strcmp(key, obj->pairs[j].key) == 0) {
                    fprintf(stderr,
                        "  dup:   %d:%d\n",
                        obj->pairs[j].line,
                        obj->pairs[j].col);
                }
            }

            ok = 0;
        }
    }

    free(skip);

    // recurse into nested objects/arrays
    for (size_t i = 0; i < obj->count; i++) {
        const char *k = obj->pairs[i].key;
        const Value *v = obj->pairs[i].value;

        char child_path[512];
        make_child_path(child_path, sizeof(child_path), path, k);

        if (!check_value(v, child_path))
            ok = 0;
    }

    return ok;
}

static int check_value(const Value *v, const char *path) {
    if (!v) return 1;

    switch (v->type) {
        case VAL_OBJECT:
            return check_object_duplicates(&v->as.object, path);

        case VAL_ARRAY: {
            int ok = 1;
            for (size_t i = 0; i < v->as.array.count; i++) {
                // path remains same for array items
                if (!check_value(v->as.array.items[i], path)) ok = 0;
            }
            return ok;
        }

        // leaf values have nothing to check here
        case VAL_NUMBER:
        case VAL_STRING:
        case VAL_BOOL:
        default:
            return 1;
    }
}