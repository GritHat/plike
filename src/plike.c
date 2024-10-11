#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>

#define MAX_LINE_LENGTH 10000
#define MAX_VARS 10000
#define MAX_FUNCS 1000
#define BUFFER_SIZE 1 << 13
#define NUM_OPERATORS 28
#define EOS '\0'
#define SPACE ' '

char current_function[64] = "";
bool current_function_returned = false;
char *dynamic_buffer[BUFFER_SIZE] = { NULL };
char *current_line = NULL;
int dynamic_buffer_index;

char *c_types[6] = {
    "int",
    "float",
    "char",
    "bool",
    "void",
    "typeerr"
};

typedef struct {
    const char* keyword;
    const char* name;
} Operator;

Operator operator[NUM_OPERATORS] = {
    {.keyword = " && ", .name = " and "},
    {.keyword = " && ", .name = " .and. "},
    {.keyword = " || ", .name = " or "},
    {.keyword = " || ", .name = " .or. "},
    {.keyword = " & ", .name = " band "},
    {.keyword = " & ", .name = " .band. "},
    {.keyword = " | ", .name = " bor "},
    {.keyword = " | ", .name = " .bor. "},
    {.keyword = " ^ ", .name = " xor "},
    {.keyword = " ^ ", .name = " .xor. "},
    {.keyword = " ! ", .name = " not "},
    {.keyword = " ! ", .name = " .not. "},
    {.keyword = " false ", .name = " false "},
    {.keyword = " false ", .name = " .false. "},
    {.keyword = " true ", .name = " true "},
    {.keyword = " true ", .name = " .true. "},
    {.keyword = " == ", .name = " is "},
    {.keyword = " == ", .name = " .is. "},
    {.keyword = " == ", .name = " equal "},
    {.keyword = " == ", .name = " .equal. "},
    {.keyword = " == ", .name = " equals "},
    {.keyword = " == ", .name = " .equals. "},
    {.keyword = " != ", .name = " isnot "},
    {.keyword = " != ", .name = " .isnot. "},
    {.keyword = " != ", .name = " notequal "},
    {.keyword = " != ", .name = " .notequal. "},
    {.keyword = " != ", .name = " notequals "},
    {.keyword = " != ", .name = " .notequals. "}
};

enum {
    CurrentLine,
    Replace,
    GP0,
    GP1,
    GP2,
};

enum {
    INT,
    FLOAT,
    CHAR,
    BOOL,
    VOID,
    TYPEERR
};

void free_buffer()
{
    for (int i = dynamic_buffer_index; i >= 0; --i)
    {
        if (dynamic_buffer[i])
        {
            free(dynamic_buffer[i]);
            dynamic_buffer[i] = NULL;
        }
    }
    dynamic_buffer_index = 0;
}

char *vprint_to_buffer(char* format, va_list args)
{
    va_list apc;
    va_copy(apc, args);
    int length = vsnprintf(NULL, 0, format, apc);
    length = ((((length + 1) * sizeof(char)) / 4) + 1) * 4;
    dynamic_buffer[dynamic_buffer_index] = malloc(length);
    vsprintf(dynamic_buffer[dynamic_buffer_index], format, args);
    va_end(apc);
    if (dynamic_buffer_index + 1 >= BUFFER_SIZE) {
        printf("buffer limit (%d) exceeded, increase BUFFER_SIZE\n", BUFFER_SIZE);
        exit(1);
    }
    return dynamic_buffer[dynamic_buffer_index++];
}

char *print_to_buffer(char* format, ...)
{
    va_list args;
    va_start (args, format);
    char *ret = vprint_to_buffer(format, args);
    va_end (args);
    return ret;
}

// Function to count leading spaces or tabs for indentation level
int get_indentation_level(const char* line) {
    int count = 0;
    while (line[count] == SPACE || line[count] == '\t') {
        count++;
    }
    return count;
}

char* strip_spaces(char* line) {
    char *ret = print_to_buffer(line);
    char *tmp = ret;
    char *d = tmp;
    do {
        while (*d == SPACE) {
            ++d;
        }
    } while ((*tmp++ = *d++));
    return ret;
}

char* rstrip_spaces(char* line) {
    char *ret = print_to_buffer(line);
    char *end = &ret[strlen(ret) - 1];
    while (*end == SPACE) {
        *end = EOS;
        --end;
    }
    return ret;
}

char* lstrip_spaces(char* line) {
    char* ret = print_to_buffer(line);
    while (*ret == SPACE) {
        ++ret;
    }
    return ret;
}

char* to_lowercase(char* line) {
    char *ret = print_to_buffer(line);
    for(char *d=ret; *d; d++) *d=tolower(*d);
    return ret;
}

char* plike_type_to_c(char *type) {
    if (!type)
        return c_types[TYPEERR]; //TODO handle typeless or missing types
    if (strstr(to_lowercase(strip_spaces(type)), "integer")) {
        return c_types[INT];
    } else if (strstr(to_lowercase(strip_spaces(type)), "real")) {
        return c_types[FLOAT];
    } else if (strstr(to_lowercase(strip_spaces(type)), "character")) {
        return c_types[CHAR];
    } else if (strstr(to_lowercase(strip_spaces(type)), "logical") || strstr(to_lowercase(strip_spaces(type)), "logic")) {
        return c_types[BOOL];
    }
    return c_types[INT]; //TODO handle typeless or missing types
}

char* sanitize_line(char* line) {
    if (!line)
        return NULL;
    char* s = print_to_buffer(line);
    char* l = to_lowercase(s);
    char *p1, *p2, *p3 = print_to_buffer(line);

    
    for (int i = 0; i < NUM_OPERATORS; ++i) {
        p1 = strstr(l, operator[i].name);
        if (p1) {
            p2 = print_to_buffer(s);
            *(p2 + ((uint64_t) p1 - (uint64_t) l)) = '\0';
            p3 = print_to_buffer("%s%s%s", p2, operator[i].keyword, s - (uint64_t)l + (uint64_t)p1 + strlen(operator[i].name));
        }
    }
    return p3;
}

void append_to_current_line(char *format, ...) {
    va_list args;
    va_start (args, format);
    char *tmp1 = vprint_to_buffer(format, args);
    va_end (args);
    if (!tmp1)
        return;
    char *tmp2 = tmp1;
    if (current_line) {
        tmp2 = print_to_buffer("%s%s", current_line, tmp1);
        free(current_line);
        current_line = NULL;
    }
    if (!tmp2)
        return;
    tmp2 =  sanitize_line(tmp2);
    int length = ((((strlen(tmp2) + 1) * sizeof(char)) / 4) + 1) * 4;
    current_line = malloc(length);
    strcpy(current_line, tmp2);
}

void append_to_current_line_no_sanitize(char *format, ...) {
    va_list args;
    va_start (args, format);
    char *tmp1 = vprint_to_buffer(format, args);
    va_end (args);
    if (!tmp1)
        return;
    char *tmp2 = tmp1;
    if (current_line) {
        tmp2 = print_to_buffer("%s%s", current_line, tmp1);
        free(current_line);
        current_line = NULL;
    }
    if (!tmp2)
        return;
    int length = ((((strlen(tmp2) + 1) * sizeof(char)) / 4) + 1) * 4;
    current_line = malloc(length);
    strcpy(current_line, tmp2);
}

// Function to print indentation based on the level
void print_indentation(int level) {
    for (int i = 0; i < level; i++) {
        append_to_current_line(" ");
    }
}

bool is_empty_or_whitespace(char* line) {
    for (int i = 0; line[i] != EOS; i++) {
        if (!isspace(line[i])) {
            return 0;
        }
    }
    return 1;
}

void current_function_return() {
    append_to_current_line("    return %s;\n", current_function);
}

void print_line() {
    if (current_line && !is_empty_or_whitespace(current_line)) {
        printf("%s\n", current_line);
        free_buffer();
        free(current_line);
        current_line = NULL;
    }
}

typedef struct {
    char name[512];
    char type[10];  // "int", "float", "char", "bool"
    char init_values[1024];
    unsigned short dimensions;
    int function;
    bool initialized;
} Variable;

Variable variables[MAX_VARS];

typedef struct {
    char name[512];
    char type[10];
    char args[2048];
    short outputs[256];
    short outputs_count;
} Function;

Function functions[MAX_FUNCS];
int var_count = 0;
int func_count = 0;

int get_variable_index_by_name(char* name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0 && (func_count == variables[i].function)){
            return i;
        }
        if (strcmp(variables[i].name, name) == 0 && (func_count == variables[i].function)) {
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

void add_variables(char* names, char* type) {
    names = strip_spaces(names);
    if (strchr(names, ',')) {
        char *name = strtok(names, ",");
        while (name) {
            add_variable(name, type);
            name = strtok(NULL, ",");
        }
    } else {
        add_variable(names, type);
    }
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

void add_arrays(char* names, char* type, unsigned short dimensions)
{
    names = strip_spaces(names);
    if (strchr(names, ',')) {
        names = strtok(names, ",");
        while (names) {
            char* name = print_to_buffer(names);
            char* end = strstr(name, "[");
            if (end)
                *end = EOS;
            add_array(lstrip_spaces(name), type, dimensions);
            names = strtok(NULL, ",");
        }
    } else {
        add_array(names, type, dimensions);
    }
}

char* get_variable_type(char* name) {
    int var_index = get_variable_index_by_name(name);
    return (var_index != -1) ? variables[var_index].type : NULL;
}

Variable* get_variable(char* name) {
    int var_index = get_variable_index_by_name(strip_spaces(name));
    return (var_index != -1) ? &variables[var_index] : NULL;
}

void scan_variable_declaration(char* line) {
    char* type = strstr(line, ":");
    if (type) {
        *type = EOS;
        type += 2;  // Skip ": "
        char variable_name[50];
        sscanf(line + 4, "%s", variable_name);  // Skip "var " and extract name
        char *end = strchr(variable_name, '[');
        if (end != NULL) {
            *end = EOS;
        }
        char *tmp = strstr(to_lowercase(strip_spaces(type)), "arrayof");
        if (tmp) {
            int dimensions = 1;
            tmp += 7;
            while ((tmp = strstr(tmp, "arrayof")) != NULL) {
                ++dimensions;
                tmp += 7;
            }
            add_arrays(line + 4, plike_type_to_c(type), dimensions);
        }
    }
}

void translate_array_initialization(Variable* array) {
    append_to_current_line("int %s[] = %s;\n", array->name, array->init_values);
}

void translate_variable_declaration(char* line, int indentation_level) {
    char* type = strstr(line, ":");
    if (type) {
        *type = EOS;
        type += 2;  // Skip ": "
        print_indentation(indentation_level);

        char *tmp = strstr(to_lowercase(strip_spaces(type)), "arrayof");
        int dimensions = 1;
        if (tmp) {
            tmp += 7;
            while ((tmp = strstr(tmp, "arrayof")) != NULL) {
                ++dimensions;
                tmp += 7;
            }
            char *variable_names = print_to_buffer(line + 4); // Skip "var " and extract name
            if (strchr(variable_names, ',')) {
                variable_names = strtok(variable_names, ",");
                while (variable_names) {
                    char* variable_name = print_to_buffer(variable_names);
                    char* variable_name_and_dims = print_to_buffer(variable_names);
                    char* end = strstr(variable_name, "[");
                    if (end)
                        *end = EOS;

                    Variable* var = get_variable(variable_name);
                    if (var && var->initialized) {
                        translate_array_initialization(var);
                    } else {
                        append_to_current_line("%s %s;\n", plike_type_to_c(type), lstrip_spaces(rstrip_spaces(variable_name_and_dims)));
                    }
                    print_indentation(indentation_level);
                    variable_names = strtok(NULL, ",");
                }
                current_line[strlen(current_line) - (indentation_level + 1)] = '\0';   
            }
        } else {
            append_to_current_line("%s %s;", plike_type_to_c(type), rstrip_spaces(line + 4));  // Skip "var "
            add_variables(line + 4, plike_type_to_c(type));
        }
    }
}

void translate_print_and_read(char* line, int indentation_level) {
    print_indentation(indentation_level);
    char* variable = strchr(line, '(') + 1;
    char* end = strchr(variable, ')');
    *end = EOS;
    char variable_name[50];
    sscanf(variable, "%s", variable_name);
    char* variable_name_end = strchr(variable_name, '[');
    if (variable_name_end != NULL)
        *variable_name_end = EOS;

    char* type = get_variable_type(variable_name);
    if (strstr(line, "print") == line) {
        if (type) {
            if (strcmp(type, "int") == 0) {
                append_to_current_line_no_sanitize("printf(\"%%d\", %s);", variable);
            } else if (strcmp(type, "float") == 0) {
                append_to_current_line_no_sanitize("printf(\"%%f\", %s);", variable);
            } else if (strcmp(type, "char") == 0) {
                append_to_current_line_no_sanitize("printf(\"%%c\", %s);", variable);
            } else if (strcmp(type, "bool") == 0) {
                // Print "true" or "false" for bool
                append_to_current_line_no_sanitize("printf(\"%%s\", %s ? \"true\" : \"false\");", variable);
            }
        }
    } else if (strstr(line, "read") == line) {
        if (type) {
            if (strcmp(type, "int") == 0) {
                append_to_current_line_no_sanitize("    scanf(\"%%d\", &%s);", variable);
            } else if (strcmp(type, "float") == 0) {
                append_to_current_line_no_sanitize("    scanf(\"%%f\", &%s);", variable);
            } else if (strcmp(type, "char") == 0) {
                append_to_current_line_no_sanitize("    scanf(\"%%c\", &%s);", variable);
            } else if (strcmp(type, "bool") == 0) {
                append_to_current_line_no_sanitize("    scanf(\"%%d\", &%s);", variable);  // bool is often read as int
            }
        }
    }
}

/*void a(char* line){
    char* name = strtok(line, " ");
    char* ret;// = malloc(strlen(line) + 1);
    //memcpy(ret, line, strlen(line));
    bool is_func;
    size_t len;
    while (name) {
        is_func = false;
        for (int i = func_count; i >= 0; --i) {
            char* name_no_parenthesis = strip_spaces(name);
            char* p = strstr(name_no_parenthesis, "(");
            if (p)
                *p = EOS;
            if (strcmp(name_no_parenthesis, functions[i].name) == 0) {
                is_func = true;
                // then check argument for out or in/out parameters to be converted to pointers & (ignore arrays)
                ret = malloc(strlen(line) + functions[i].outputs_count);
                char* params = strstr(line, "(");
                uint64_t gap1 = (uint64_t) params - (uint64_t) line;
                if (params) {
                    ++params; // skip (
                    params = print_to_buffer(params);
                    p = strstr(params, ")");
                    *p = EOS;
                    char* param = strtok(params, ",");
                    int param_count = 1; // we keep 0 for no outputs
                    memcpy(ret, line, gap1); //maybe +1
                    while (param) {
                        uint64_t gap2 = (uint64_t) param - (uint64_t) params;
                        for (int j = 0; j < functions[i].outputs_count; ++j) {
                            if (functions[i].outputs[j] == param_count) {
                                len = strlen()
                                memcpy(ret + gap1, line + )
                                memcpy(ret + gap1, "&", 1);
                                memcpy(ret + gap1 + 1,)
                                
                                break;
                            }

                        }

                        char* arg = strtok(functions[i].args, ",");
                        while (arg) {
                            char* arg_name = strstr(arg, "out");
                            if (arg_name) {
                                while (*arg_name == ':' || *arg_name == ' ') //skipping all spaces and possible out:
                                    ++arg_name;
                                
                            }
                            if (strstr(, "out")) {
                            
                            }
                        }
                        param = strtok(NULL, ",");
                    }
                }
                break;
            }
        }
        if (!is_func) {
            len = strlen(ret);
            memcpy(ret + len, " ", 1);
            memcpy(ret + len + 1, name, strlen(name));
            memcpy(ret + len + 1 + strlen(name), "\0", 1);
        }
        name = strtok(NULL, " ");
    }

    char* assign = strstr(line, ":=");
    if (assign) {
        char* buf = malloc((uint64_t)assign - (uint64_t)line);
        memcpy(buf, line, (uint64_t)assign - (uint64_t)line);
        if (strcmp(strip_spaces(buf), current_function) == 0) {

        } else {
        }
    }
}*/


/*
Scan all functions and procedures and add them to the array including arg string and (out, in/out) args count and wheter is implicit or explicit argument type
scan internal functions and procedures declarations and store variables (done)
translate function header with proper argument type if it was implicit by checking each name in the arg string and if it is missing the type check name against variables array to find it, mark the relative variable as declared and skip translating its declaration
check each line for function or procedure calls and add & to out and in/out params which are not arrays
check = which are not := and replace them with == then replace := with = if assign op setting is 0 else do nothing (= means assing, == means equal)
*/

void translate_function_call(char* line, int indentation_level) {
    print_indentation(indentation_level);
    if (strstr(line, ":=") == NULL && line[0] != '\n') {  // It's a function/procedure call
        append_to_current_line("%s;", line);  // Add semicolon after function call
    }
}

void translate_control_structure(char* line, int indentation_level) {
    print_indentation(indentation_level);
    if (strstr(line, "if") == line) {
        char* condition = strstr(line, "if") + 2;
        char* then = strstr(condition, "then");
        if (then) *then = EOS;
        append_to_current_line("if (%s) {", condition);
    } else if (strstr(line, "else") == line) {
        append_to_current_line("} else {");
    } else if (strstr(line, "endif") == line || 
               strstr(line, "endwhile") == line || 
               strstr(line, "endfor") == line) {
        append_to_current_line("}");
    } else if (strstr(line, "while") == line) {
        char* condition = strstr(line, "while") + 5;
        char* do_keyword = strstr(condition, "do");
        if (do_keyword) *do_keyword = EOS;
        append_to_current_line("while (%s) {", condition);
    } else if (strstr(line, "for") == line) {
        char* var = strstr(line, "for") + 3;
        char* start = strstr(var, ":=") + 2;
        char* to = strstr(start, "to");
        char* do_keyword = strstr(to, "do");
        if (to) *to = EOS;
        if (do_keyword) *do_keyword = EOS;
        to += 2;
        char var_name[50] = {0};
        sscanf(var, "%s", var_name);
        append_to_current_line("for (%s = %s; %s <= %s; ++%s) {", var_name, start, var_name, to, var_name);
    } else if (strstr(line, "repeat") == line) {
        append_to_current_line("do {");
    } else if (strstr(line, "until") == line) {
        char* condition = strstr(line, "until") + 5;
        append_to_current_line("} while (!(%s));", condition);
    }
}

void translate_function_or_procedure(char* line, int indentation_level) {
    print_indentation(indentation_level);
    if (strstr(line, "function") == line || strstr(line, "procedure") == line) {
        char* name;
        char* params;
        int c_type = TYPEERR;
        if (strstr(line, "function") == line) {
            name = strstr(line, "function") + 9;
            params = strchr(name, '(');
            char* return_type = strstr(params, "):");
            if (return_type) {
                *return_type = EOS;
                return_type += 2;
                if (strstr(return_type, "integer")) {
                    append_to_current_line("int ");
                    c_type = INT;
                } else if (strstr(return_type, "real")) {
                    append_to_current_line("float ");
                    c_type = FLOAT;
                } else if (strstr(return_type, "logical") || strstr(return_type, "logic")) {
                    append_to_current_line("bool ");
                    c_type = BOOL;
                } else if (strstr(return_type, "character")) {
                    append_to_current_line("char ");
                    c_type = CHAR;
                }
            } else {
                append_to_current_line("void ");
                c_type = VOID;
            }
        } else {
            name = strstr(line, "procedure") + 10;
            params = strchr(name, '(');
            append_to_current_line("void ");
            c_type = VOID;
        }

        *params = EOS;
        append_to_current_line("%s(", name);
        if (strstr(line, "function")) {
            sscanf(name, "%s", current_function);
            current_function_returned = false;
        }
        else
            current_function[0] = EOS;

        params++; // Skip '('

        sscanf(name, "%s", functions[func_count].name);
        sprintf(functions[func_count].type, "%s", c_types[c_type]);
        sprintf(functions[func_count].args, "%s", params);

        char* param = strtok(params, ",");
        while (param) {
            char param_type[50];
            char param_name[50];

            sscanf(param, "%*s %[^:]: %199c", param_name, param_type);
            char *inner_type = strstr(strip_spaces(param_type), "arrayof");
            if (inner_type)
                append_to_current_line("%s %s", plike_type_to_c(inner_type + 7), param_name); // skip arrayof
            else if (strstr(param, "out")) {
                append_to_current_line("%s* %s", plike_type_to_c(param_type), param_name);
            } else {
                append_to_current_line("%s %s", plike_type_to_c(param_type), param_name);    // Value for in
            }

            param = strtok(NULL, ",");
            if (param) {
                append_to_current_line(", ");
            }
        }

        append_to_current_line(") {");
    } else if (strstr(line, "endfunction") == line || strstr(line, "endprocedure") == line) {
        if (strstr(line, "endfunction") == line && !current_function_returned) {
            current_function_return();
        }
        append_to_current_line("}");
        current_function[0] = EOS;  // Clear current function name
    }
}

void scan_assignment(char* line) {
    char* assign = strstr(line, ":=");
    if (assign) {
        char* tmp = print_to_buffer(line);
        //sscanf(line, "%s", tmp);
        *assign = '=';
        *(assign + 1) = SPACE;
        char* tmpptr = strstr(tmp, ":=");
        if (tmpptr) {
            *tmpptr = EOS;
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
    char* assign = strstr(line, ":=");
    if (assign) {
        char* tmp = print_to_buffer(line);
        //sscanf(line, "%s", tmp);
        *assign = '=';
        *(assign + 1) = SPACE;
        char* tmpptr = strstr(tmp, ":=");
        if (tmpptr) {
            *tmpptr = EOS;
        }
        tmpptr = strchr(tmp, '[');
        if (tmpptr == NULL) // we now know if it is accessing a member or initializing the whole array
        {
            Variable* var = get_variable(tmp);
            if (var && var->dimensions) {
                return;
            }
        }
    }
    print_indentation(indentation_level);
    append_to_current_line("%s;", line);
}

void translate_return(char* line, int indentation_level) {
    print_indentation(indentation_level);
    if (current_function[0] != EOS) {
        char* name = strip_spaces(line);
        name = strstr(name, "return") + 6; // skip return
        if (name && *name != '\0' && *name != '\n')
            append_to_current_line("%s;", line);
        else
            append_to_current_line("return %s;", current_function); // prevent wrong indentation, a cleaner solution would be printing the indentation only after checking something will be printed, maybe use a gp_buffer to store everything and than print only at the end of each line
        current_function_returned = true;
    } else {
        append_to_current_line("%s;", line);
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
        append_to_current_line("int main() {");
    } else if (strstr(line, "endprogram") == line) {
        append_to_current_line("    return 0;\n}");
    } else if (strstr(line, ":=")) {
        translate_assignment(line, indentation_level);
    } else if (strstr(line, "begin") == line || strstr(line, "end") == line) {
        // Ignore 'begin' and 'end' keywords
        
    } else if (strstr(line, "read") == line || strstr(line, "print") == line) {
        translate_print_and_read(line, indentation_level);
    } else if (is_empty_or_whitespace(line)) {
        append_to_current_line("");
    } else if (strstr(line, ":=") == NULL) {
        translate_function_call(line, indentation_level);
    } else {
        append_to_current_line("%s", line);
    }
    print_line();
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

void include_headers() {
    printf("#include <stdbool.h>\n");
    printf("#include <math.h>\n");
    printf("#include <stdio.h>\n\n");
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
    include_headers();
    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\n")] = 0;  // Remove newline
        translate_line(line);
    }
    return 0;
}