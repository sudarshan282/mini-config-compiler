# mini config compiler

# overview
A compiler that converts a simple, user-friendly configuration language into a valid JSON file. It checks for errors and ensures the generated configuration is correct and safe to use.

# features
lexical analysis (tokenization)
syntax parsing (builds AST)
semantic checks (duplicate key detection)
exact line and column error reporting
JSON code generation
grouped lexical error detection

# example i/p with corresponding JSON file o/p
server {
  host = "localhost"
  port = 8080
  debug = true
}

database {
  engine = "postgres"
  pool = 20
}


{
  "server": {
    "host": "localhost",
    "port": 8080,
    "debug": true
  },
  "database": {
    "engine": "postgres",
    "pool": 20
  }
}

# example with error
server {
  port = 8080
  port = 9000
}

database {
  pool = @@!!
}

Semantic error: duplicate key 'server.port'
  first: 2:3
  dup:   3:3

Lex error at 7:10: Unexpected characters "@@!!"
Compilation aborted due to errors.

# working steps
-lexer converts input text into tokens
-parser builds an Abstract Syntax Tree (AST)
-semantic analysis checks logical errors
-code generator produces JSON output


