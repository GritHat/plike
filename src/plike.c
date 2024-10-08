#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1000
#define MAX_VARS 100

char current_function[64] = "";

// Function to count leading spaces or tabs for indentation level
int get_indentation_level(const char* line) {
    int count = 0;
    while (line[count] == ' ' || line[count] == '\t') {
        count++;
    }
    return count;
}

// Function to print indentation based on the level
void print_indentation(int level) {
    for (int i = 0; i < level; i++) {
        printf(" ");
    }
}

bool is_empty_or_whitespace(char* line) {
    for (int i = 0; line[i] != '\0'; i++) {
        if (!isspace(line[i])) {
            return 0;
        }
    }
    return 1;
}

typedef struct {
    char name[50];
    char type[10];  // "int", "float", "char", "bool"
    char init_values[1024];
    unsigned short dimensions;
    int function;
    bool initialized;
} Variable;

Variable variables[MAX_VARS];
int var_count = 0;
int func_count = 0;

int get_variable_index_by_name(char* name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0 && func_count == variables[i].function) {
            return i;
        }
    }
    return -1;
}

void add_variable(char* name, char* type) {
    strcpy(variables[var_count].name, name);
    strcpy(variables[var_count].type, type);
    variables[var_count].dimensions = 0;
    variables[var_count].function = func_count;
    var_count++;
}

void add_array(char* name, char* type, unsigned short dimensions) {
    int index = get_variable_index_by_name(name);
    if (index == -1)
        index = var_count++;
    strcpy(variables[index].name, name);
    strcpy(variables[index].type, type);
    variables[index].dimensions = dimensions;
    variables[index].function = func_count;
}

char* get_variable_type(char* name) {
    int var_index = get_variable_index_by_name(name);
    return (var_index != -1) ? variables[var_index].type : NULL;
}

Variable* get_variable(char* name) {
    int var_index = get_variable_index_by_name(name);
    return (var_index != -1) ? &variables[var_index] : NULL;
}

void scan_variable_declaration(char* line) {
    char* type = strstr(line, ":");
    if (type) {
        *type = '\0';
        type += 2;  // Skip ": "
        char variable_name[50];
        sscanf(line + 4, "%s", variable_name);  // Skip "var " and extract name
        char *end = strchr(variable_name, '[');
        if (end != NULL) {
            *end = '\0';
        }
        if (strstr(type, "array of")) {
            int dimensions = 1;
            char * tmp = type + 8;
            while ((tmp = strstr(tmp, "array of")) != NULL) {
                ++dimensions;
                tmp += 8;
            }
            char* arrayType = type + 9;  // Skip "array of "
            if (strstr(arrayType, "integer")) {
                //printf("int %s;\n", line + 4);
                add_array(variable_name, "int", dimensions);
            } else if (strstr(arrayType, "real")) {
                //printf("float %s;\n", line + 4);
                add_array(variable_name, "float", dimensions);
            } else if (strstr(arrayType, "logical")) {
                //printf("bool %s;\n", line + 4);
                add_array(variable_name, "bool", dimensions);
            } else if (strstr(arrayType, "character")) {
                //printf("char %s;\n", line + 4);
                add_array(variable_name, "char", dimensions);
            }
        }
    }
}

void translate_array_initialization(Variable* array) {
    printf("int %s[] = %s;\n", array->name, array->init_values);
}

void translate_variable_declaration(char* line, int indentation_level) {
    char* type = strstr(line, ":");
    if (type) {
        *type = '\0';
        type += 2;  // Skip ": "
        print_indentation(indentation_level);
        char variable_name[50];
        sscanf(line + 4, "%s", variable_name);  // Skip "var " and extract name
        char *end = strchr(variable_name, '[');
        if (end != NULL) {
            *end = '\0';
        }
        if (strstr(type, "array of")) {
            int dimensions = 1;
            char * tmp = type + 8;
            while ((tmp = strstr(tmp, "array of")) != NULL) {
                ++dimensions;
                tmp += 8;
            }
            Variable* var = get_variable(variable_name);
            if (var && var->initialized) {
                translate_array_initialization(var);
            } else {
                printf("int %s;\n", line + 4);
            }
        }
        else if (strstr(type, "integer")) {
            printf("int %s;\n", line + 4);  // Skip "var "
            add_variable(variable_name, "int");
        } else if (strstr(type, "real")) {
            printf("float %s;\n", line + 4);
            add_variable(variable_name, "float");
        } else if (strstr(type, "logical")) {
            printf("bool %s;\n", line + 4);
            add_variable(variable_name, "bool");
        } else if (strstr(type, "character")) {
            printf("char %s;\n", line + 4);
            add_variable(variable_name, "char");
        } 
    }
}

void translate_print_and_read(char* line, int indentation_level) {
    print_indentation(indentation_level);
    char* variable = strchr(line, '(') + 1;
    char* end = strchr(variable, ')');
    *end = '\0';
    char variable_name[50];
    sscanf(variable, "%s", variable_name);
    char* variable_name_end = strchr(variable_name, '[');
    if (variable_name_end != NULL)
        *variable_name_end = '\0';

    char* type = get_variable_type(variable_name);
    if (strstr(line, "print") == line) {
        if (type) {
            if (strcmp(type, "int") == 0) {
                printf("printf(\"%%d\", %s);\n", variable);
            } else if (strcmp(type, "float") == 0) {
                printf("printf(\"%%f\", %s);\n", variable);
            } else if (strcmp(type, "char") == 0) {
                printf("printf(\"%%c\", %s);\n", variable);
            } else if (strcmp(type, "bool") == 0) {
                // Print "true" or "false" for bool
                printf("printf(\"%%s\", %s ? \"true\" : \"false\");\n", variable);
            }
        }
    } else if (strstr(line, "read") == line) {
        if (type) {
            if (strcmp(type, "int") == 0) {
                printf("    scanf(\"%%d\", &%s);\n", variable);
            } else if (strcmp(type, "float") == 0) {
                printf("    scanf(\"%%f\", &%s);\n", variable);
            } else if (strcmp(type, "char") == 0) {
                printf("    scanf(\"%%c\", &%s);\n", variable);
            } else if (strcmp(type, "bool") == 0) {
                printf("    scanf(\"%%d\", &%s);\n", variable);  // bool is often read as int
            }
        }
    }
}

void translate_function_call(char* line, int indentation_level) {
    print_indentation(indentation_level);
    if (strstr(line, ":=") == NULL && line[0] != '\n') {  // It's a function/procedure call
        printf("%s;\n", line);  // Add semicolon after function call
    }
}

void translate_control_structure(char* line, int indentation_level) {
    print_indentation(indentation_level);
    if (strstr(line, "if") == line) {
        char* condition = strstr(line, "if") + 2;
        char* then = strstr(condition, "then");
        if (then) *then = '\0';
        printf("if (%s) {\n", condition);
    } else if (strstr(line, "else") == line) {
        printf("} else {\n");
    } else if (strstr(line, "endif") == line || 
               strstr(line, "endwhile") == line || 
               strstr(line, "endfor") == line) {
        printf("}\n");
    } else if (strstr(line, "while") == line) {
        char* condition = strstr(line, "while") + 5;
        char* do_keyword = strstr(condition, "do");
        if (do_keyword) *do_keyword = '\0';
        printf("while (%s) {\n", condition);
    } else if (strstr(line, "for") == line) {
        char* var = strstr(line, "for") + 3;
        char* start = strstr(var, ":=") + 2;
        char* to = strstr(start, "to");
        char* do_keyword = strstr(to, "do");
        if (to) *to = '\0';
        if (do_keyword) *do_keyword = '\0';
        to += 2;
        char var_name[50] = {0};
        sscanf(var, "%s", var_name);
        printf("for (%s = %s; %s <= %s; ++%s) {\n", var_name, start, var_name, to, var_name);
    } else if (strstr(line, "repeat") == line) {
        printf("do {\n");
    } else if (strstr(line, "until") == line) {
        char* condition = strstr(line, "until") + 5;
        printf("} while (!(%s));\n", condition);
    }
}

void translate_function_or_procedure(char* line, int indentation_level) {
    print_indentation(indentation_level);
    if (strstr(line, "function") == line || strstr(line, "procedure") == line) {
        char* name;
        char* params;
        if (strstr(line, "function") == line) {
            name = strstr(line, "function") + 9;
            params = strchr(name, '(');
            char* return_type = strstr(params, "):");
            if (return_type) {
                *return_type = '\0';
                return_type += 2;
                if (strstr(return_type, "integer")) {
                    printf("int ");
                } else if (strstr(return_type, "real")) {
                    printf("float ");
                } else if (strstr(return_type, "logical")) {
                    printf("bool ");
                } else if (strstr(return_type, "character")) {
                    printf("char ");
                }
            } else {
                printf("void ");
            }
        } else {
            name = strstr(line, "procedure") + 10;
            params = strchr(name, '(');
            printf("void ");
        }

        *params = '\0';
        printf("%s(", name);
        if (strstr(line, "function"))
            sscanf(name, "%s", current_function);
        else
            current_function[0] = '\0';

        params++; // Skip '('
        char* param = strtok(params, ",");
        while (param) {
            char param_type[50];
            char param_name[50];

            sscanf(param, "%*s %[^:]: %s", param_name, param_type);

            if (strstr(param, "in/out")) {
                if (strstr(param_type, "array")) {
                    printf("int %s", param_name);  // Correct 1D array
                } else {
                    printf("int* %s", param_name);   // Pointer for in/out
                }
            } else if (strstr(param, "out")) {
                if (strstr(param_type, "array")) {
                    printf("int %s", param_name);  // Correct 1D array
                } else {
                    printf("int* %s", param_name);   // Pointer for out
                }
            } else if (strstr(param, "in")) {
                if (strstr(param_type, "array")) {
                    printf("int %s", param_name);  // Correct 1D array
                } else {
                    printf("int %s", param_name);    // Value for in
                }
            }

            param = strtok(NULL, ",");
            if (param) {
                printf(", ");
            }
        }

        printf(") {\n");
    } else if (strstr(line, "endfunction") == line || strstr(line, "endprocedure") == line) {
        printf("//translating endfunction or endprocedure\n");
        printf("}\n");
        current_function[0] = '\0';  // Clear current function name
    }
}

void scan_assignment(char* line) {
    char* assign = strstr(line, ":=");
    if (assign) {
        char tmp[128];
        sscanf(line, "%s", tmp);
        *assign = '=';
        *(assign + 1) = ' ';
        char* tmpptr = strstr(tmp, ":=");
        if (tmpptr) {
            *tmpptr = '\0';
        }
        tmpptr = strchr(tmp, '[');
        if (tmpptr == NULL) // we now know if it is accessing a member or initializing the whole array
        {
            Variable* var = get_variable(tmp);
            if (var && var->dimensions) {
                var->initialized = true;
                char* init_value = strchr(line, '{');
                if (init_value) {
                    sprintf(var->init_values, "%s", init_value);
                }
            }
        }
    }
}

void translate_assignment(char* line, int indentation_level) {
    print_indentation(indentation_level);
    char* assign = strstr(line, ":=");
    if (assign) {
        char tmp[128];
        sscanf(line, "%s", tmp);
        *assign = '=';
        *(assign + 1) = ' ';
        char* tmpptr = strstr(tmp, ":=");
        if (tmpptr) {
            *tmpptr = '\0';
        }
        tmpptr = strchr(tmp, '[');
        if (tmpptr == NULL) // we now know if it is accessing a member or initializing the whole array
        {
            Variable* var = get_variable(tmp);
            if (var && var->dimensions) {
                return;
            }
            // Check if this is a function return assignment
            else if (strcmp(tmp, current_function) == 0) {
                printf("return %s;\n", assign + 2);  // +2 to skip "= "
                return;
            }
        }
        
    }
    printf("%s;\n", line);
}

void translate_return(char* line, int indentation_level) {
    print_indentation(indentation_level);
    if (current_function[0] != '\0') {
        // This is a bare "return" in a function, so we don't need to do anything
        // The actual return statement was already generated in translate_assignment
    } else {
        printf("%s;\n", line);
    }
}

void translate_line(char* line) {
    // Strip leading whitespace
    int indentation_level = get_indentation_level(line);
    line += indentation_level;

    if (strstr(line, "var") == line) {
        translate_variable_declaration(line, indentation_level);
    } else if (strstr(line, "return")) {
        translate_return(line, indentation_level);
    } else if (strstr(line, "if") == line || strstr(line, "else") == line || 
               strstr(line, "endif") == line || strstr(line, "while") == line || 
               strstr(line, "endwhile") == line || strstr(line, "for") == line || 
               strstr(line, "endfor") == line || strstr(line, "repeat") == line ||
               strstr(line, "until") == line) {
        translate_control_structure(line, indentation_level);
    } else if (strstr(line, "function") == line || strstr(line, "procedure") == line || 
               strstr(line, "endfunction") == line || strstr(line, "endprocedure") == line) {
        translate_function_or_procedure(line, indentation_level);
        ++func_count;
    } else if (strstr(line, "program") == line) {
        ++func_count;
        printf("int main() {\n");
    } else if (strstr(line, "endprogram") == line) {
        printf("    return 0;\n}\n");
    } else if (strstr(line, ":=")) {
        translate_assignment(line, indentation_level);
    } else if (strstr(line, "begin") == line || strstr(line, "end") == line) {
        // Ignore 'begin' and 'end' keywords
        
    } else if (strstr(line, "read") == line || strstr(line, "print") == line) {
        translate_print_and_read(line, indentation_level);
    } else if (is_empty_or_whitespace(line)) {
        printf("\n");
    } else if (strstr(line, ":=") == NULL) {
        translate_function_call(line, indentation_level);
    } else {
        printf("    %s\n", line);
    }
}

void scan_arrays(char * line) {
    // Strip leading whitespace
    int indentation_level = get_indentation_level(line);
    line += indentation_level;

    if (strstr(line, "var") == line) {
        scan_variable_declaration(line);
    } else if (strstr(line, "function") == line || strstr(line, "procedure") == line || 
               strstr(line, "endfunction") == line || strstr(line, "endprocedure") == line ||
               strstr(line, "program") == line) {
        ++func_count;
    } else if (strstr(line, ":=")) { 
        scan_assignment(line);
    }
}

void reset_globals() {
    func_count = 0;
}

int main() {
    char line[MAX_LINE_LENGTH];
    reset_globals();
    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\n")] = 0;  // Remove newline
        scan_arrays(line);
    }
    rewind(stdin);

    reset_globals();
    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\n")] = 0;  // Remove newline
        translate_line(line);
    }
    return 0;
}