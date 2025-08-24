#!/usr/bin/awk -f
# mscri.awk - AWK implementation of Mscri language interpreter

BEGIN {
    # Initialize interpreter state
    delete variables
    delete tokens
    delete keywords
    
    # Set up keywords
    keywords["let"] = 1
    keywords["if"] = 1
    keywords["then"] = 1
    keywords["else"] = 1
    keywords["endif"] = 1
    keywords["while"] = 1
    keywords["do"] = 1
    keywords["endwhile"] = 1
    keywords["for"] = 1
    keywords["to"] = 1
    keywords["step"] = 1
    keywords["endfor"] = 1
    keywords["function"] = 1
    keywords["endfunction"] = 1
    keywords["return"] = 1
    keywords["print"] = 1
    keywords["and"] = 1
    keywords["or"] = 1
    keywords["not"] = 1
    keywords["true"] = 1
    keywords["false"] = 1
    
    # REPL mode if no file specified
    if (ARGC == 1) {
        run_repl()
    }
}

# File mode - process each line
{
    if (FILENAME != "") {
        execute_line($0)
    }
}

function run_repl() {
    print "Mscri Interpreter v1.0 (AWK)"
    print "Type 'exit' to quit"
    print ""
    
    while (1) {
        printf "? "
        fflush()
        
        if ((getline line) <= 0) {
            break
        }
        
        if (line == "exit") {
            break
        }
        
        if (length(line) == 0) {
            continue
        }
        
        execute_line(line)
    }
    
    print "Goodbye!"
}

function execute_line(line) {
    # Remove comments
    gsub(/\/\/.*$/, "", line)
    gsub(/\/\*.*\*\//, "", line)
    
    # Trim whitespace
    gsub(/^[ \t\r\n]+|[ \t\r\n]+$/, "", line)
    
    if (length(line) == 0) {
        return
    }
    
    # Tokenize and parse
    delete tokens
    token_count = tokenize(line)
    
    if (token_count > 0) {
        token_pos = 1
        execute_statement()
    }
}

function tokenize(line,    i, c, token, in_string, string_char, escape) {
    delete tokens
    token_count = 0
    i = 1
    
    while (i <= length(line)) {
        c = substr(line, i, 1)
        
        # Skip whitespace
        if (c ~ /[ \t\r\n]/) {
            i++
            continue
        }
        
        # String literals
        if (c == "\"" || c == "'") {
            string_char = c
            token = ""
            i++
            
            while (i <= length(line) && substr(line, i, 1) != string_char) {
                c = substr(line, i, 1)
                if (c == "\\") {
                    i++
                    if (i <= length(line)) {
                        escape = substr(line, i, 1)
                        if (escape == "n") token = token "\n"
                        else if (escape == "t") token = token "\t"
                        else if (escape == "\\") token = token "\\"
                        else token = token escape
                    }
                } else {
                    token = token c
                }
                i++
            }
            
            if (i <= length(line)) i++ # Skip closing quote
            tokens[++token_count] = "STRING:" token
            continue
        }
        
        # Numbers
        if (c ~ /[0-9]/) {
            token = ""
            while (i <= length(line) && substr(line, i, 1) ~ /[0-9.]/) {
                token = token substr(line, i, 1)
                i++
            }
            tokens[++token_count] = "NUMBER:" token
            continue
        }
        
        # Identifiers and keywords
        if (c ~ /[a-zA-Z_]/) {
            token = ""
            while (i <= length(line) && substr(line, i, 1) ~ /[a-zA-Z0-9_]/) {
                token = token substr(line, i, 1)
                i++
            }
            
            if (token in keywords) {
                tokens[++token_count] = "KEYWORD:" token
            } else {
                tokens[++token_count] = "IDENTIFIER:" token
            }
            continue
        }
        
        # Two-character operators
        if (i < length(line)) {
            two_char = substr(line, i, 2)
            if (two_char == "==" || two_char == "!=" || two_char == "<=" || two_char == ">=") {
                tokens[++token_count] = "OPERATOR:" two_char
                i += 2
                continue
            }
        }
        
        # Single-character operators and delimiters
        if (c ~ /[+\-*\/%^=<>(),'"]/) {
            if (c ~ /[+\-*\/%^=<>]/) {
                tokens[++token_count] = "OPERATOR:" c
            } else {
                tokens[++token_count] = "DELIMITER:" c
            }
            i++
            continue
        }
        
        # Skip unknown characters
        i++
    }
    
    return token_count
}

function get_token_type(token) {
    split(token, parts, ":")
    return parts[1]
}

function get_token_value(token) {
    split(token, parts, ":")
    return parts[2]
}

function current_token() {
    if (token_pos <= token_count) {
        return tokens[token_pos]
    }
    return "EOF:"
}

function advance_token() {
    if (token_pos <= token_count) {
        token_pos++
    }
}

function execute_statement() {
    token = current_token()
    type = get_token_type(token)
    value = get_token_value(token)
    
    if (type == "EOF") {
        return
    }
    
    if (type == "KEYWORD") {
        if (value == "let") {
            execute_assignment()
        } else if (value == "print") {
            execute_print()
        } else if (value == "if") {
            execute_if()
        }
    } else {
        # Expression statement
        result = parse_expression()
    }
}

function execute_assignment() {
    advance_token() # skip 'let'
    
    token = current_token()
    if (get_token_type(token) != "IDENTIFIER") {
        print "Error: Expected variable name"
        return
    }
    
    var_name = get_token_value(token)
    advance_token()
    
    token = current_token()
    if (get_token_type(token) != "OPERATOR" || get_token_value(token) != "=") {
        print "Error: Expected '='"
        return
    }
    advance_token()
    
    value = parse_expression()
    variables[var_name] = value
}

function execute_print() {
    advance_token() # skip 'print'
    value = parse_expression()
    print value
}

function execute_if() {
    advance_token() # skip 'if'
    
    condition = parse_expression()
    
    token = current_token()
    if (get_token_type(token) != "KEYWORD" || get_token_value(token) != "then") {
        print "Error: Expected 'then'"
        return
    }
    advance_token()
    
    # Simple if - execute next statement if condition is true
    if (to_number(condition) != 0) {
        execute_statement()
    }
    
    # Skip to endif (simplified)
    while (token_pos <= token_count) {
        token = current_token()
        if (get_token_type(token) == "KEYWORD" && get_token_value(token) == "endif") {
            advance_token()
            break
        }
        advance_token()
    }
}

function parse_expression() {
    return parse_logical_or()
}

function parse_logical_or(    left, op) {
    left = parse_logical_and()
    
    while (1) {
        token = current_token()
        if (get_token_type(token) != "KEYWORD" || get_token_value(token) != "or") {
            break
        }
        
        advance_token()
        right = parse_logical_and()
        left = evaluate_binary_op(left, "or", right)
    }
    
    return left
}

function parse_logical_and(    left, right) {
    left = parse_equality()
    
    while (1) {
        token = current_token()
        if (get_token_type(token) != "KEYWORD" || get_token_value(token) != "and") {
            break
        }
        
        advance_token()
        right = parse_equality()
        left = evaluate_binary_op(left, "and", right)
    }
    
    return left
}

function parse_equality(    left, op, right) {
    left = parse_comparison()
    
    while (1) {
        token = current_token()
        if (get_token_type(token) != "OPERATOR") {
            break
        }
        
        op = get_token_value(token)
        if (op != "==" && op != "!=") {
            break
        }
        
        advance_token()
        right = parse_comparison()
        left = evaluate_binary_op(left, op, right)
    }
    
    return left
}

function parse_comparison(    left, op, right) {
    left = parse_addition()
    
    while (1) {
        token = current_token()
        if (get_token_type(token) != "OPERATOR") {
            break
        }
        
        op = get_token_value(token)
        if (op != "<" && op != ">" && op != "<=" && op != ">=") {
            break
        }
        
        advance_token()
        right = parse_addition()
        left = evaluate_binary_op(left, op, right)
    }
    
    return left
}

function parse_addition(    left, op, right) {
    left = parse_multiplication()
    
    while (1) {
        token = current_token()
        if (get_token_type(token) != "OPERATOR") {
            break
        }
        
        op = get_token_value(token)
        if (op != "+" && op != "-") {
            break
        }
        
        advance_token()
        right = parse_multiplication()
        left = evaluate_binary_op(left, op, right)
    }
    
    return left
}

function parse_multiplication(    left, op, right) {
    left = parse_power()
    
    while (1) {
        token = current_token()
        if (get_token_type(token) != "OPERATOR") {
            break
        }
        
        op = get_token_value(token)
        if (op != "*" && op != "/" && op != "%") {
            break
        }
        
        advance_token()
        right = parse_power()
        left = evaluate_binary_op(left, op, right)
    }
    
    return left
}

function parse_power(    left, op, right) {
    left = parse_unary()
    
    while (1) {
        token = current_token()
        if (get_token_type(token) != "OPERATOR" || get_token_value(token) != "^") {
            break
        }
        
        advance_token()
        right = parse_unary()
        left = evaluate_binary_op(left, "^", right)
    }
    
    return left
}

function parse_unary(    op, operand) {
    token = current_token()
    type = get_token_type(token)
    value = get_token_value(token)
    
    if (type == "OPERATOR" && (value == "-" || value == "+")) {
        advance_token()
        operand = parse_unary()
        return evaluate_unary_op(value, operand)
    } else if (type == "KEYWORD" && value == "not") {
        advance_token()
        operand = parse_unary()
        return evaluate_unary_op("not", operand)
    }
    
    return parse_primary()
}

function parse_primary(    token, type, value, expr) {
    token = current_token()
    type = get_token_type(token)
    value = get_token_value(token)
    
    if (type == "NUMBER") {
        advance_token()
        return value + 0 # Convert to number
    }
    
    if (type == "STRING") {
        advance_token()
        return value
    }
    
    if (type == "KEYWORD") {
        if (value == "true") {
            advance_token()
            return 1
        } else if (value == "false") {
            advance_token()
            return 0
        }
    }
    
    if (type == "IDENTIFIER") {
        if (value in variables) {
            advance_token()
            return variables[value]
        } else {
            print "Error: Variable '" value "' not defined"
            advance_token()
            return 0
        }
    }
    
    if (type == "DELIMITER" && value == "(") {
        advance_token() # skip '('
        expr = parse_expression()
        
        token = current_token()
        if (get_token_type(token) == "DELIMITER" && get_token_value(token) == ")") {
            advance_token() # skip ')'
        }
        
        return expr
    }
    
    return 0
}

function evaluate_binary_op(left, op, right,    l, r) {
    if (op == "+") {
        # String concatenation if either operand is a string
        if (!is_numeric(left) || !is_numeric(right)) {
            return left "" right
        } else {
            return (left + 0) + (right + 0)
        }
    }
    
    # Convert to numbers for other operations
    l = to_number(left)
    r = to_number(right)
    
    if (op == "-") return l - r
    if (op == "*") return l * r
    if (op == "/") return (r != 0) ? l / r : 0
    if (op == "%") return (r != 0) ? l % r : 0
    if (op == "^") return l ^ r
    if (op == "==") return (l == r) ? 1 : 0
    if (op == "!=") return (l != r) ? 1 : 0
    if (op == "<") return (l < r) ? 1 : 0
    if (op == ">") return (l > r) ? 1 : 0
    if (op == "<=") return (l <= r) ? 1 : 0
    if (op == ">=") return (l >= r) ? 1 : 0
    if (op == "and") return (l && r) ? 1 : 0
    if (op == "or") return (l || r) ? 1 : 0
    
    return 0
}

function evaluate_unary_op(op, operand,    val) {
    val = to_number(operand)
    
    if (op == "-") return -val
    if (op == "+") return val
    if (op == "not") return val ? 0 : 1
    
    return 0
}

function is_numeric(str) {
    return str ~ /^-?[0-9]+\.?[0-9]*$/
}

function to_number(val) {
    if (is_numeric(val)) {
        return val + 0
    }
    return 0
}

# Built-in functions
function mscri_len(str) {
    return length(str)
}

function mscri_int(val) {
    return int(to_number(val))
}

function mscri_str(val) {
    return val ""
}
