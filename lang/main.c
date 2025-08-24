/* mscri.c - C implementation of Mscri language interpreter */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_TOKEN_LEN 256
#define MAX_VARS 100
#define MAX_STACK_SIZE 1000
#define MAX_LINE_LEN 1024

typedef enum {
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD,
    TOKEN_OPERATOR,
    TOKEN_DELIMITER,
    TOKEN_EOF,
    TOKEN_NEWLINE
} TokenType;

typedef enum {
    VALUE_NUMBER,
    VALUE_STRING
} ValueType;

typedef struct {
    TokenType type;
    char lexeme[MAX_TOKEN_LEN];
    double number;
    int line;
    int column;
} Token;

typedef struct {
    ValueType type;
    union {
        double number;
        char* string;
    } data;
} Value;

typedef struct {
    char name[MAX_TOKEN_LEN];
    Value value;
} Variable;

typedef struct {
    char* source;
    int position;
    int line;
    int column;
    int length;
} Lexer;

typedef struct {
    Variable vars[MAX_VARS];
    int var_count;
} Environment;

/* Keywords */
static const char* keywords[] = {
    "let", "if", "then", "else", "endif", "while", "do", "endwhile",
    "for", "to", "step", "endfor", "function", "endfunction", "return",
    "print", "and", "or", "not", "true", "false", NULL
};

/* Global environment */
static Environment env = {0};

/* Function prototypes */
void init_lexer(Lexer* lexer, char* source);
Token next_token(Lexer* lexer);
Value parse_expression(Lexer* lexer, Token* current);
void execute_statement(Lexer* lexer, Token* current);
void run_repl();
Value create_number(double num);
Value create_string(const char* str);
void free_value(Value* val);
Variable* find_variable(const char* name);
void set_variable(const char* name, Value val);
int is_keyword(const char* str);
void skip_whitespace(Lexer* lexer);
void skip_comment(Lexer* lexer);
char current_char(Lexer* lexer);
char peek_char(Lexer* lexer);
void advance(Lexer* lexer);
void print_value(Value val);
Value evaluate_binary_op(Value left, const char* op, Value right);
Value evaluate_unary_op(const char* op, Value operand);

/* Lexer implementation */
void init_lexer(Lexer* lexer, char* source) {
    lexer->source = source;
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->length = strlen(source);
}

char current_char(Lexer* lexer) {
    if (lexer->position >= lexer->length) {
        return '\0';
    }
    return lexer->source[lexer->position];
}

char peek_char(Lexer* lexer) {
    int peek_pos = lexer->position + 1;
    if (peek_pos >= lexer->length) {
        return '\0';
    }
    return lexer->source[peek_pos];
}

void advance(Lexer* lexer) {
    if (lexer->position < lexer->length) {
        if (lexer->source[lexer->position] == '\n') {
            lexer->line++;
            lexer->column = 1;
        } else {
            lexer->column++;
        }
        lexer->position++;
    }
}

void skip_whitespace(Lexer* lexer) {
    while (current_char(lexer) == ' ' || current_char(lexer) == '\t' || current_char(lexer) == '\r') {
        advance(lexer);
    }
}

void skip_comment(Lexer* lexer) {
    if (current_char(lexer) == '/' && peek_char(lexer) == '/') {
        while (current_char(lexer) != '\n' && current_char(lexer) != '\0') {
            advance(lexer);
        }
    } else if (current_char(lexer) == '/' && peek_char(lexer) == '*') {
        advance(lexer); // skip '/'
        advance(lexer); // skip '*'
        while (!(current_char(lexer) == '*' && peek_char(lexer) == '/') && current_char(lexer) != '\0') {
            advance(lexer);
        }
        if (current_char(lexer) == '*') {
            advance(lexer); // skip '*'
            advance(lexer); // skip '/'
        }
    }
}

int is_keyword(const char* str) {
    for (int i = 0; keywords[i] != NULL; i++) {
        if (strcmp(str, keywords[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

Token next_token(Lexer* lexer) {
    Token token;
    memset(&token, 0, sizeof(Token));
    
    while (current_char(lexer) != '\0') {
        skip_whitespace(lexer);
        
        if (current_char(lexer) == '/' && (peek_char(lexer) == '/' || peek_char(lexer) == '*')) {
            skip_comment(lexer);
            continue;
        }
        
        token.line = lexer->line;
        token.column = lexer->column;
        
        char c = current_char(lexer);
        
        if (c == '\n') {
            token.type = TOKEN_NEWLINE;
            strcpy(token.lexeme, "\\n");
            advance(lexer);
            return token;
        }
        
        if (c == '"' || c == '\'') {
            char quote = c;
            advance(lexer); // skip opening quote
            
            int i = 0;
            while (current_char(lexer) != quote && current_char(lexer) != '\0' && i < MAX_TOKEN_LEN - 1) {
                if (current_char(lexer) == '\\') {
                    advance(lexer);
                    char escaped = current_char(lexer);
                    switch (escaped) {
                        case 'n': token.lexeme[i++] = '\n'; break;
                        case 't': token.lexeme[i++] = '\t'; break;
                        case '\\': token.lexeme[i++] = '\\'; break;
                        default: token.lexeme[i++] = escaped; break;
                    }
                } else {
                    token.lexeme[i++] = current_char(lexer);
                }
                advance(lexer);
            }
            
            if (current_char(lexer) == quote) {
                advance(lexer); // skip closing quote
            }
            
            token.lexeme[i] = '\0';
            token.type = TOKEN_STRING;
            return token;
        }
        
        if (isdigit(c)) {
            int i = 0;
            int has_dot = 0;
            
            while ((isdigit(current_char(lexer)) || (current_char(lexer) == '.' && !has_dot)) && i < MAX_TOKEN_LEN - 1) {
                if (current_char(lexer) == '.') {
                    has_dot = 1;
                }
                token.lexeme[i++] = current_char(lexer);
                advance(lexer);
            }
            
            token.lexeme[i] = '\0';
            token.type = TOKEN_NUMBER;
            token.number = atof(token.lexeme);
            return token;
        }
        
        if (isalpha(c) || c == '_') {
            int i = 0;
            
            while ((isalnum(current_char(lexer)) || current_char(lexer) == '_') && i < MAX_TOKEN_LEN - 1) {
                token.lexeme[i++] = current_char(lexer);
                advance(lexer);
            }
            
            token.lexeme[i] = '\0';
            token.type = is_keyword(token.lexeme) ? TOKEN_KEYWORD : TOKEN_IDENTIFIER;
            return token;
        }
        
        // Two-character operators
        if (c == '=' && peek_char(lexer) == '=') {
            strcpy(token.lexeme, "==");
            token.type = TOKEN_OPERATOR;
            advance(lexer);
            advance(lexer);
            return token;
        }
        
        if (c == '!' && peek_char(lexer) == '=') {
            strcpy(token.lexeme, "!=");
            token.type = TOKEN_OPERATOR;
            advance(lexer);
            advance(lexer);
            return token;
        }
        
        if (c == '<' && peek_char(lexer) == '=') {
            strcpy(token.lexeme, "<=");
            token.type = TOKEN_OPERATOR;
            advance(lexer);
            advance(lexer);
            return token;
        }
        
        if (c == '>' && peek_char(lexer) == '=') {
            strcpy(token.lexeme, ">=");
            token.type = TOKEN_OPERATOR;
            advance(lexer);
            advance(lexer);
            return token;
        }
        
        // Single-character operators and delimiters
        if (strchr("+-*/%^=<>(),'", c)) {
            token.lexeme[0] = c;
            token.lexeme[1] = '\0';
            token.type = strchr("+-*/%^=<>", c) ? TOKEN_OPERATOR : TOKEN_DELIMITER;
            advance(lexer);
            return token;
        }
        
        // Skip unknown characters
        advance(lexer);
    }
    
    token.type = TOKEN_EOF;
    strcpy(token.lexeme, "EOF");
    return token;
}

/* Value management */
Value create_number(double num) {
    Value val;
    val.type = VALUE_NUMBER;
    val.data.number = num;
    return val;
}

Value create_string(const char* str) {
    Value val;
    val.type = VALUE_STRING;
    val.data.string = malloc(strlen(str) + 1);
    strcpy(val.data.string, str);
    return val;
}

void free_value(Value* val) {
    if (val->type == VALUE_STRING && val->data.string) {
        free(val->data.string);
        val->data.string = NULL;
    }
}

void print_value(Value val) {
    if (val.type == VALUE_NUMBER) {
        if (val.data.number == (int)val.data.number) {
            printf("%.0f", val.data.number);
        } else {
            printf("%g", val.data.number);
        }
    } else if (val.type == VALUE_STRING) {
        printf("%s", val.data.string);
    }
}

/* Variable management */
Variable* find_variable(const char* name) {
    for (int i = 0; i < env.var_count; i++) {
        if (strcmp(env.vars[i].name, name) == 0) {
            return &env.vars[i];
        }
    }
    return NULL;
}

void set_variable(const char* name, Value val) {
    Variable* var = find_variable(name);
    if (var) {
        free_value(&var->value);
        var->value = val;
    } else if (env.var_count < MAX_VARS) {
        strcpy(env.vars[env.var_count].name, name);
        env.vars[env.var_count].value = val;
        env.var_count++;
    }
}

/* Expression evaluation */
Value evaluate_binary_op(Value left, const char* op, Value right) {
    if (strcmp(op, "+") == 0) {
        if (left.type == VALUE_STRING || right.type == VALUE_STRING) {
            // String concatenation
            char* result = malloc(1000); // Simple allocation
            if (left.type == VALUE_STRING && right.type == VALUE_STRING) {
                sprintf(result, "%s%s", left.data.string, right.data.string);
            } else if (left.type == VALUE_STRING) {
                sprintf(result, "%s%g", left.data.string, right.data.number);
            } else {
                sprintf(result, "%g%s", left.data.number, right.data.string);
            }
            Value val = create_string(result);
            free(result);
            return val;
        } else {
            return create_number(left.data.number + right.data.number);
        }
    }
    
    // Numeric operations only
    double l = (left.type == VALUE_NUMBER) ? left.data.number : 0;
    double r = (right.type == VALUE_NUMBER) ? right.data.number : 0;
    
    if (strcmp(op, "-") == 0) return create_number(l - r);
    if (strcmp(op, "*") == 0) return create_number(l * r);
    if (strcmp(op, "/") == 0) return create_number(l / r);
    if (strcmp(op, "%") == 0) return create_number(fmod(l, r));
    if (strcmp(op, "^") == 0) return create_number(pow(l, r));
    if (strcmp(op, "==") == 0) return create_number(l == r ? 1 : 0);
    if (strcmp(op, "!=") == 0) return create_number(l != r ? 1 : 0);
    if (strcmp(op, "<") == 0) return create_number(l < r ? 1 : 0);
    if (strcmp(op, ">") == 0) return create_number(l > r ? 1 : 0);
    if (strcmp(op, "<=") == 0) return create_number(l <= r ? 1 : 0);
    if (strcmp(op, ">=") == 0) return create_number(l >= r ? 1 : 0);
    if (strcmp(op, "and") == 0) return create_number((l && r) ? 1 : 0);
    if (strcmp(op, "or") == 0) return create_number((l || r) ? 1 : 0);
    
    return create_number(0);
}

Value evaluate_unary_op(const char* op, Value operand) {
    double val = (operand.type == VALUE_NUMBER) ? operand.data.number : 0;
    
    if (strcmp(op, "-") == 0) return create_number(-val);
    if (strcmp(op, "+") == 0) return create_number(val);
    if (strcmp(op, "not") == 0) return create_number(val ? 0 : 1);
    
    return create_number(0);
}

/* Forward declarations for parsing */
Value parse_primary(Lexer* lexer, Token* current);
Value parse_unary(Lexer* lexer, Token* current);
Value parse_power(Lexer* lexer, Token* current);
Value parse_multiplication(Lexer* lexer, Token* current);
Value parse_addition(Lexer* lexer, Token* current);
Value parse_comparison(Lexer* lexer, Token* current);
Value parse_equality(Lexer* lexer, Token* current);
Value parse_logical_and(Lexer* lexer, Token* current);
Value parse_logical_or(Lexer* lexer, Token* current);

void skip_newlines(Token* current, Lexer* lexer) {
    while (current->type == TOKEN_NEWLINE) {
        *current = next_token(lexer);
    }
}

Value parse_primary(Lexer* lexer, Token* current) {
    if (current->type == TOKEN_NUMBER) {
        Value val = create_number(current->number);
        *current = next_token(lexer);
        return val;
    }
    
    if (current->type == TOKEN_STRING) {
        Value val = create_string(current->lexeme);
        *current = next_token(lexer);
        return val;
    }
    
    if (current->type == TOKEN_KEYWORD && (strcmp(current->lexeme, "true") == 0 || strcmp(current->lexeme, "false") == 0)) {
        Value val = create_number(strcmp(current->lexeme, "true") == 0 ? 1 : 0);
        *current = next_token(lexer);
        return val;
    }
    
    if (current->type == TOKEN_IDENTIFIER) {
        Variable* var = find_variable(current->lexeme);
        if (var) {
            Value val;
            if (var->value.type == VALUE_STRING) {
                val = create_string(var->value.data.string);
            } else {
                val = create_number(var->value.data.number);
            }
            *current = next_token(lexer);
            return val;
        } else {
            printf("Error: Variable '%s' not defined\n", current->lexeme);
            *current = next_token(lexer);
            return create_number(0);
        }
    }
    
    if (current->type == TOKEN_DELIMITER && strcmp(current->lexeme, "(") == 0) {
        *current = next_token(lexer);
        Value val = parse_expression(lexer, current);
        if (current->type == TOKEN_DELIMITER && strcmp(current->lexeme, ")") == 0) {
            *current = next_token(lexer);
        }
        return val;
    }
    
    return create_number(0);
}

Value parse_unary(Lexer* lexer, Token* current) {
    if (current->type == TOKEN_OPERATOR && (strcmp(current->lexeme, "-") == 0 || strcmp(current->lexeme, "+") == 0)) {
        char op[MAX_TOKEN_LEN];
        strcpy(op, current->lexeme);
        *current = next_token(lexer);
        Value operand = parse_unary(lexer, current);
        return evaluate_unary_op(op, operand);
    }
    
    if (current->type == TOKEN_KEYWORD && strcmp(current->lexeme, "not") == 0) {
        *current = next_token(lexer);
        Value operand = parse_unary(lexer, current);
        return evaluate_unary_op("not", operand);
    }
    
    return parse_primary(lexer, current);
}

Value parse_power(Lexer* lexer, Token* current) {
    Value left = parse_unary(lexer, current);
    
    while (current->type == TOKEN_OPERATOR && strcmp(current->lexeme, "^") == 0) {
        char op[MAX_TOKEN_LEN];
        strcpy(op, current->lexeme);
        *current = next_token(lexer);
        Value right = parse_unary(lexer, current);
        left = evaluate_binary_op(left, op, right);
    }
    
    return left;
}

Value parse_multiplication(Lexer* lexer, Token* current) {
    Value left = parse_power(lexer, current);
    
    while (current->type == TOKEN_OPERATOR && 
           (strcmp(current->lexeme, "*") == 0 || strcmp(current->lexeme, "/") == 0 || strcmp(current->lexeme, "%") == 0)) {
        char op[MAX_TOKEN_LEN];
        strcpy(op, current->lexeme);
        *current = next_token(lexer);
        Value right = parse_power(lexer, current);
        left = evaluate_binary_op(left, op, right);
    }
    
    return left;
}

Value parse_addition(Lexer* lexer, Token* current) {
    Value left = parse_multiplication(lexer, current);
    
    while (current->type == TOKEN_OPERATOR && 
           (strcmp(current->lexeme, "+") == 0 || strcmp(current->lexeme, "-") == 0)) {
        char op[MAX_TOKEN_LEN];
        strcpy(op, current->lexeme);
        *current = next_token(lexer);
        Value right = parse_multiplication(lexer, current);
        left = evaluate_binary_op(left, op, right);
    }
    
    return left;
}

Value parse_comparison(Lexer* lexer, Token* current) {
    Value left = parse_addition(lexer, current);
    
    while (current->type == TOKEN_OPERATOR && 
           (strcmp(current->lexeme, "<") == 0 || strcmp(current->lexeme, ">") == 0 ||
            strcmp(current->lexeme, "<=") == 0 || strcmp(current->lexeme, ">=") == 0)) {
        char op[MAX_TOKEN_LEN];
        strcpy(op, current->lexeme);
        *current = next_token(lexer);
        Value right = parse_addition(lexer, current);
        left = evaluate_binary_op(left, op, right);
    }
    
    return left;
}

Value parse_equality(Lexer* lexer, Token* current) {
    Value left = parse_comparison(lexer, current);
    
    while (current->type == TOKEN_OPERATOR && 
           (strcmp(current->lexeme, "==") == 0 || strcmp(current->lexeme, "!=") == 0)) {
        char op[MAX_TOKEN_LEN];
        strcpy(op, current->lexeme);
        *current = next_token(lexer);
        Value right = parse_comparison(lexer, current);
        left = evaluate_binary_op(left, op, right);
    }
    
    return left;
}

Value parse_logical_and(Lexer* lexer, Token* current) {
    Value left = parse_equality(lexer, current);
    
    while (current->type == TOKEN_KEYWORD && strcmp(current->lexeme, "and") == 0) {
        *current = next_token(lexer);
        Value right = parse_equality(lexer, current);
        left = evaluate_binary_op(left, "and", right);
    }
    
    return left;
}

Value parse_logical_or(Lexer* lexer, Token* current) {
    Value left = parse_logical_and(lexer, current);
    
    while (current->type == TOKEN_KEYWORD && strcmp(current->lexeme, "or") == 0) {
        *current = next_token(lexer);
        Value right = parse_logical_and(lexer, current);
        left = evaluate_binary_op(left, "or", right);
    }
    
    return left;
}

Value parse_expression(Lexer* lexer, Token* current) {
    return parse_logical_or(lexer, current);
}

/* Statement execution */
void execute_statement(Lexer* lexer, Token* current) {
    skip_newlines(current, lexer);
    
    if (current->type == TOKEN_EOF) {
        return;
    }
    
    if (current->type == TOKEN_KEYWORD) {
        if (strcmp(current->lexeme, "let") == 0) {
            *current = next_token(lexer);
            if (current->type == TOKEN_IDENTIFIER) {
                char var_name[MAX_TOKEN_LEN];
                strcpy(var_name, current->lexeme);
                *current = next_token(lexer);
                
                if (current->type == TOKEN_OPERATOR && strcmp(current->lexeme, "=") == 0) {
                    *current = next_token(lexer);
                    Value val = parse_expression(lexer, current);
                    set_variable(var_name, val);
                }
            }
        }
        else if (strcmp(current->lexeme, "print") == 0) {
            *current = next_token(lexer);
            Value val = parse_expression(lexer, current);
            print_value(val);
            printf("\n");
            free_value(&val);
        }
        else if (strcmp(current->lexeme, "if") == 0) {
            *current = next_token(lexer);
            Value condition = parse_expression(lexer, current);
            
            if (current->type == TOKEN_KEYWORD && strcmp(current->lexeme, "then") == 0) {
                *current = next_token(lexer);
                
                if (condition.type == VALUE_NUMBER && condition.data.number != 0) {
                    // Execute then block (simplified - just one statement)
                    skip_newlines(current, lexer);
                    if (current->type != TOKEN_KEYWORD || strcmp(current->lexeme, "endif") != 0) {
                        execute_statement(lexer, current);
                    }
                }
                
                // Skip to endif
                while (current->type != TOKEN_EOF && 
                       !(current->type == TOKEN_KEYWORD && strcmp(current->lexeme, "endif") == 0)) {
                    *current = next_token(lexer);
                }
                
                if (current->type == TOKEN_KEYWORD && strcmp(current->lexeme, "endif") == 0) {
                    *current = next_token(lexer);
                }
            }
            
            free_value(&condition);
        }
    }
}

/* REPL */
void run_repl() {
    char line[MAX_LINE_LEN];
    
    printf("Mscri Interpreter v1.0 (C)\n");
    printf("Type 'exit' to quit\n\n");
    
    while (1) {
        printf("mscri> ");
        fflush(stdout);
        
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }
        
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        if (strcmp(line, "exit") == 0) {
            break;
        }
        
        if (strlen(line) == 0) {
            continue;
        }
        
        Lexer lexer;
        init_lexer(&lexer, line);
        
        Token current = next_token(&lexer);
        execute_statement(&lexer, &current);
    }
    
    printf("Goodbye!\n");
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        // File mode
        FILE* file = fopen(argv[1], "r");
        if (!file) {
            printf("Error: Cannot open file '%s'\n", argv[1]);
            return 1;
        }
        
        fseek(file, 0, SEEK_END);
        long length = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        char* source = malloc(length + 1);
        fread(source, 1, length, file);
        source[length] = '\0';
        fclose(file);
        
        Lexer lexer;
        init_lexer(&lexer, source);
        
        Token current = next_token(&lexer);
        while (current.type != TOKEN_EOF) {
            execute_statement(&lexer, &current);
        }
        
        free(source);
    } else {
        // REPL mode
        run_repl();
    }
    
    // Cleanup
    for (int i = 0; i < env.var_count; i++) {
        free_value(&env.vars[i].value);
    }
    
    return 0;
}
