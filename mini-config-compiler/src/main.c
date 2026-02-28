#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "semantic.h"
#include "codegen.h"
static char* read_entire_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < 0) { fclose(f); return NULL; }

    char *buf = (char*)malloc((size_t)size + 1);
    if (!buf) { fclose(f); return NULL; }

    size_t readn = fread(buf, 1, (size_t)size, f);
    fclose(f);

    buf[readn] = '\0';
    return buf;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file.mcfg>\n", argv[0]);
        return 1;
    }

    char *source = read_entire_file(argv[1]);
    if (!source) {
        fprintf(stderr, "Error: cannot read file '%s'\n", argv[1]);
        return 1;
    }

    Parser ps;
    parser_init(&ps, source);

    Program prog = parser_parse_program(&ps);

    if (ps.had_error) {
        fprintf(stderr, "Parsing failed.\n");
        program_free(&prog);
        free(source);
        return 1;
    }

    int sem_ok = semantic_check_program(&prog);
    if (!sem_ok) {
        fprintf(stderr, "Compilation aborted due to semantic errors.\n");
        program_free(&prog);
        free(source);
        return 1;
    }

    printf("Compilation successful \n\nGenerated JSON:\n\n");

    codegen_json(stdout, &prog);

    program_free(&prog);
    free(source);
    return 0;
}