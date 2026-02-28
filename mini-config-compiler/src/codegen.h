#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"

// Generate JSON output from program AST
void codegen_json(FILE *out, const Program *prog);

#endif