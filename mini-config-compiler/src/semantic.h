#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

// Returns 1 if OK, 0 if semantic errors found.
int semantic_check_program(const Program *p);

#endif