/* --- Includes --- */
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"
#include "time.h"


/* --- Types --- */
#define bool char
#define true 1
#define false 0
#define null 0
#define pnumber double
#define pnumber_i long
typedef struct stack_element stack_element;
struct stack_element {
    stack_element * next;
    char * value;
};
typedef struct var_element var_element;
struct var_element {
    var_element * next;
    char * value;
    char * name;
};


/* --- Constants --- */
#define OS_TYPE 1 /* 1 - Unix, 2 - MS-DOS */
#define VERSION "1.0"
#if OS_TYPE == 1
    #define OS "Unix"
#elif OS_TYPE == 2
    #define OS "MS-DOS"
#endif
#define MAXLINELENGTH 255   /* Maximum length of a line (all characters after 255 are not loaded) */
#define MAXINPUTLENGTH 1024 /* Maximum length of user input */
#define EPSILON 0.000001


/* --- Global Variables --- */
char filename[255];
char * file_contents = null;
bool display_memory_information = false;
stack_element * stack = null;
var_element * variables = null;
bool show_pushpops = false;


/* --- Function Predefinitions --- */
void check_args(int argc, char** argv);
void display_version();
void display_help();
void load_source_file(char* path);
void error(char* message);
void warning(char* message);
void eval(char* source);
void print_substr(char* source, size_t from, size_t to, bool trim);
bool comp_substr(char* source, size_t from, size_t to, char* compare_to);
void copy_substr(char* destination, char* origin, size_t from, size_t to);
void stack_push(char* value, size_t from, size_t to, bool trim, bool pushempty);
int eval_reserved_word(char* source, size_t token_start, size_t token_end);
stack_element * stack_pop();
void delete_element(stack_element * se);
void num_to_str(char* destination, pnumber number);
void set_var_value(char* var, char* value);
void get_var_value(char* var);
void polaris_setup();


/* --- Main --- */
int main(int argc, char** argv){
    check_args(argc, argv);
    load_source_file(filename);
    polaris_setup();
    eval(file_contents);
    free(file_contents);
    return 0;
}


/* --- Functions --- */
void polaris_setup(){
    srand(time(null) * clock());
}

void check_args(int argc, char** argv){
    if(argc > 1) {
        unsigned int i;
        for(i = 1; i < argc; ++i){
            #if OS_TYPE == 1
            if(strcmp(argv[i], "-v") == 0){
                display_version();
            }
            else if(strcmp(argv[i], "-h") == 0){
                display_help();
            }
            else if(strcmp(argv[i], "-m") == 0){
                display_memory_information = true;
            }
            else if(strcmp(argv[i], "-p") == 0){
                show_pushpops = true;
            }
            #elif OS_TYPE == 2
            if(strcmp(argv[i], "\\v") == 0){
                display_version();
            }
            else if(strcmp(argv[i], "\\h") == 0){
                display_help();
            }
            else if(strcmp(argv[i], "\\m") == 0){
                display_memory_information = true;
            }
            else if(strcmp(argv[i], "\\p") == 0){
                show_pushpops = true;
            }
            #endif
            else{
                strcpy(filename, argv[i]);
                if(i < argc - 1){
                    warning("some switches after the filename have been ignored.");
                }
                return;
            }
        }
    }
}

void display_version()
{
    printf("This is Polaris for %s version %s.\r\n", OS, VERSION);
    puts("Copyright 2020, Martin del Rio (www.lartu.net).");
    exit(0);
}

void display_help()
{
    puts("Usage:");
    puts("  polaris <source file>|<switch>");
    puts("Switches:");
    #if OS_TYPE == 1
    puts("  -v              Display Polaris version information.");
    puts("  -h              Display this help.");
    puts("  -m              Display memory information.");
    puts("  -p              Show push and pops during execution.");
    puts("Complete documentation for Polaris should be found on this");
    puts("system using the 'man polaris' command. If you have access");
    puts("to the internet, the documentation can also be found online");
    puts("at www.lartu.net/projects/polaris.");
    #elif OS_TYPE == 2
    puts("  /v              Display Polaris version information.");
    puts("  /h              Display this help.");
    puts("  /m              Display memory information.");
    puts("  /p              Show push and pops during execution.");
    puts("Complete documentation for Polaris should be found under");
    puts("the Polaris directory on this system. If you have access");
    puts("to the internet, the documentation can also be found online");
    puts("at www.lartu.net/projects/polaris.");
    #endif
    exit(0);
}

void load_source_file(char* path)
{
    FILE* file_pointer;
    int bufferLength = MAXLINELENGTH;
    char buffer[MAXLINELENGTH];
    size_t file_length = 0;
    file_pointer = fopen(path, "r");
    if(file_pointer)
    {
        size_t file_size;
        size_t bytes_read = 0;
        /* Measure file */
        while(fgets(buffer, bufferLength, file_pointer))
        {
            file_length += strlen(buffer);
        }
        /* Declare file buffer */
        file_size = sizeof(char) * (file_length + 1);
            /* That +1 is because I may sometimes want to access two characters at a time. */
        file_contents = malloc(file_size);
        if(display_memory_information)
        {
            printf("%lu bytes allocated for source file.\r\n", (unsigned long)file_size);
        }
        /* Load file */
        fseek(file_pointer, 0, SEEK_SET);
        bytes_read = fread(file_contents, 1, file_size, file_pointer);
        if(display_memory_information)
        {
            printf("%lu bytes read from source file.\r\n", (unsigned long)bytes_read);
        }
        fclose(file_pointer);
    }
    else
    {
        error("couldn't load the requested file.");
    }
}

void error(char* message)
{
    printf("Polaris error: %s\r\n", message);
    if(file_contents != null) free(file_contents);
    exit(1);
}

void warning(char* message)
{
    printf("Polaris warning: %s\r\n", message);
}

void eval(char* source)
{
    size_t token_start = 0;
    size_t token_end;
    size_t code_length = strlen(source)+1;
    bool in_comment = false;
    unsigned int in_block_level = 0;
    bool in_quoted = false;
    /* Find and evaluate tokens */
    char current_char;
    char next_char;
    size_t i;
    for(i = 0; i < code_length; ++i){
        current_char = source[i];
        next_char = source[i + 1];
        if(!in_quoted && current_char == '/' && next_char == '*')
        {
            in_comment = true;
            ++i;
        }
        else if(!in_quoted && current_char == '*' && next_char == '/')
        {
            in_comment = false;
            ++i;
            token_start = i + 1;
        }
        else if(current_char == '\\' && next_char == '"')
        {
            ++i;
        }
        else if(!in_comment && !in_quoted && current_char == '(')
        {
            if(in_block_level == 0){
                token_start = i;
            }
            in_block_level++;
        }
        else if(in_block_level != 0 && !in_comment && !in_quoted && current_char == ')')
        {
            in_block_level--;
            token_end = i+1;
            if(in_block_level == 0 && token_start < token_end){
                stack_push(source, token_start+1, token_end-1, true, true);
                token_start = i+1;
            }
        }
        else if(in_block_level == 0 && !in_comment && !in_quoted && current_char == '"')
        {
            in_quoted = true;
            token_start = i;
        }
        else if(in_block_level == 0 && !in_comment && in_quoted && current_char == '"')
        {
            in_quoted = false;
            token_end = i+1;
            if(token_start < token_end){
                stack_push(source, token_start+1, token_end-1, false, true);
            }
            token_start = i+1;
        }
        else if(
            in_block_level == 0 && !in_quoted && !in_comment
            && (current_char == ' ' || current_char == '\n' || current_char == '\t' || current_char == '\0'))
        {
            token_end = i;
            if(token_start < token_end){
                /* Acá evalúo el token */
                if(eval_reserved_word(source, token_start, token_end) != 0){
                    return;
                }
            }
            token_start = i;
        }
    }
}

bool str_is_num(char* source, size_t from, size_t to){
    bool already_found_sign = false;
    bool already_found_point = false;
    size_t numbers_before_point = 0;
    size_t numbers_after_point = 0;
    size_t i;
    for(i = from; i < to; ++i){
        if(!already_found_sign && source[i] == '-'){
            already_found_sign = true;
        }
        else if(!already_found_point && source[i] == '.'){
            already_found_point = true;
        }else if(
            source[i] == '0' || source[i] == '1' 
            || source[i] == '2' || source[i] == '3'
            || source[i] == '4' || source[i] == '5'
            || source[i] == '6' || source[i] == '7'
            || source[i] == '8' || source[i] == '9'
            )
        {
            already_found_sign = true;
            if(already_found_point){
                numbers_after_point++;
            }else{
                numbers_before_point++;
            }
        }else{
            return false;
        }
    }
    if(numbers_before_point == 0){
        return false;
    }
    if(already_found_point && numbers_after_point == 0){
        return false;
    }
    return true;
}

int eval_reserved_word(char* source, size_t token_start, size_t token_end){
    /* Find real start (trim) */
    size_t i;
    for(i = token_start; i < token_end; ++i){
        if(source[i] == ' ' || source[i] == '\n' || source[i] == '\t'){
            token_start++;
        }else{
            break;
        }
    }
    /* Find real end (trim) */
    for(i = token_end-1; i >= token_start; --i){
        if(source[i] == ' ' || source[i] == '\n' || source[i] == '\t'){
            token_end--;
        }else{
            break;
        }
    }
    /* print */
    if(comp_substr(source, token_start, token_end, "print")){
        stack_element * value = stack_pop();
        size_t val_len = strlen((*value).value)+1;
        source = (*value).value;
        for(i = 0; i < val_len; ++i){
            if(source[i] == '\\' && i < val_len && source[i+1] == 'n'){
                printf("\n");
                ++i;
            }else if(source[i] == '\\' && i < val_len && source[i+1] == 'r'){
                printf("\r");
                ++i;
            }else if(source[i] == '\\' && i < val_len && source[i+1] == 't'){
                printf("\t");
                ++i;
            }else if(source[i] == '\\' && i < val_len && source[i+1] == 'b'){
                printf("\b");
                ++i;
            }else if(source[i] == '\\' && i < val_len && source[i+1] == 'a'){
                printf("\a");
                ++i;
            }else if(source[i] == '\\' && i < val_len && source[i+1] == 'v'){
                printf("\v");
                ++i;
            }else if(source[i] == '\\' && i < val_len && source[i+1] == 'f'){
                printf("\f");
                ++i;
            }else if(source[i] == '\\' && i < val_len && source[i+1] == '\\'){
                printf("\\");
                ++i;
            }else if(source[i] == '\\' && i < val_len && source[i+1] == '"'){
                printf("\"");
                ++i;
            }else{
                printf("%c", source[i]);
            }
        }
        delete_element(value);
    }
    /* +  */
    else if(comp_substr(source, token_start, token_end, "+")){
        stack_element * value2 = stack_pop();
        stack_element * value1 = stack_pop();
        pnumber result;
        char result_s[50];
        if(
            !str_is_num((*value1).value, 0, strlen((*value1).value))
            || !str_is_num((*value2).value, 0, strlen((*value2).value))
        ){
            error("trying to operate arithmetically with a non-numerical value.");
        }
        result = atof((*value1).value) + atof((*value2).value);
        num_to_str(result_s, result);
        delete_element(value2);
        delete_element(value1);
        stack_push(result_s, 0, 50, true, false);
    }
    /* - */
    else if(comp_substr(source, token_start, token_end, "-")){
        stack_element * value2 = stack_pop();
        stack_element * value1 = stack_pop();
        pnumber result;
        char result_s[50];
        if(
            !str_is_num((*value1).value, 0, strlen((*value1).value))
            || !str_is_num((*value2).value, 0, strlen((*value2).value))
        ){
            error("trying to operate arithmetically with a non-numerical value.");
        }
        result = atof((*value1).value) - atof((*value2).value);
        num_to_str(result_s, result);
        delete_element(value2);
        delete_element(value1);
        stack_push(result_s, 0, 50, true, false);
    }
    /* * */
    else if(comp_substr(source, token_start, token_end, "*")){
        stack_element * value2 = stack_pop();
        stack_element * value1 = stack_pop();
        pnumber result;
        char result_s[50];
        if(
            !str_is_num((*value1).value, 0, strlen((*value1).value))
            || !str_is_num((*value2).value, 0, strlen((*value2).value))
        ){
            error("trying to operate arithmetically with a non-numerical value.");
        }
        result = atof((*value1).value) * atof((*value2).value);
        num_to_str(result_s, result);
        delete_element(value2);
        delete_element(value1);
        stack_push(result_s, 0, 50, true, false);
    }
    /* / */
    else if(comp_substr(source, token_start, token_end, "/")){
        stack_element * value2 = stack_pop();
        stack_element * value1 = stack_pop();
        pnumber result;
        char result_s[50];
        if(
            !str_is_num((*value1).value, 0, strlen((*value1).value))
            || !str_is_num((*value2).value, 0, strlen((*value2).value))
        ){
            error("trying to operate arithmetically with a non-numerical value.");
        }
        result = atof((*value1).value) / atof((*value2).value);
        num_to_str(result_s, result);
        delete_element(value2);
        delete_element(value1);
        stack_push(result_s, 0, 50, true, false);
    }
    /* % */
    else if(comp_substr(source, token_start, token_end, "%")){
        stack_element * value2 = stack_pop();
        stack_element * value1 = stack_pop();
        pnumber result;
        char result_s[50];
        if(
            !str_is_num((*value1).value, 0, strlen((*value1).value))
            || !str_is_num((*value2).value, 0, strlen((*value2).value))
        ){
            error("trying to operate arithmetically with a non-numerical value.");
        }
        result = (pnumber_i)atof((*value1).value) % (pnumber_i)atof((*value2).value);
        num_to_str(result_s, result);
        delete_element(value2);
        delete_element(value1);
        stack_push(result_s, 0, 50, true, false);
    }
    /* // */
    else if(comp_substr(source, token_start, token_end, "//")){
        stack_element * value2 = stack_pop();
        stack_element * value1 = stack_pop();
        char result_s[50];
        pnumber result;
        if(
            !str_is_num((*value1).value, 0, strlen((*value1).value))
            || !str_is_num((*value2).value, 0, strlen((*value2).value))
        ){
            error("trying to operate arithmetically with a non-numerical value.");
        }
        result = (pnumber_i)(atof((*value1).value) / atof((*value2).value));
        num_to_str(result_s, result);
        delete_element(value2);
        delete_element(value1);
        stack_push(result_s, 0, 50, true, false);
    }
    /* ** */
    else if(comp_substr(source, token_start, token_end, "**")){
        stack_element * value2 = stack_pop();
        stack_element * value1 = stack_pop();
        pnumber result;
        char result_s[50];
        if(
            !str_is_num((*value1).value, 0, strlen((*value1).value))
            || !str_is_num((*value2).value, 0, strlen((*value2).value))
        ){
            error("trying to operate arithmetically with a non-numerical value.");
        }
        result = pow(atof((*value1).value), atof((*value2).value));
        num_to_str(result_s, result);
        delete_element(value2);
        delete_element(value1);
        stack_push(result_s, 0, 50, true, false);
    }
    /* sin */
    else if(comp_substr(source, token_start, token_end, "sin")){
        stack_element * value1 = stack_pop();
        pnumber result;
        char result_s[50];
        if(
            !str_is_num((*value1).value, 0, strlen((*value1).value))
        ){
            error("trying to operate arithmetically with a non-numerical value.");
        }
        result = sin(atof((*value1).value));
        num_to_str(result_s, result);
        delete_element(value1);
        stack_push(result_s, 0, 50, true, false);
    }
    /* cos */
    else if(comp_substr(source, token_start, token_end, "cos")){
        stack_element * value1 = stack_pop();
        pnumber result;
        char result_s[50];
        if(
            !str_is_num((*value1).value, 0, strlen((*value1).value))
        ){
            error("trying to operate arithmetically with a non-numerical value.");
        }
        result = cos(atof((*value1).value));
        num_to_str(result_s, result);
        delete_element(value1);
        stack_push(result_s, 0, 50, true, false);
    }
    /* tan */
    else if(comp_substr(source, token_start, token_end, "tan")){
        stack_element * value1 = stack_pop();
        pnumber result;
        char result_s[50];
        if(
            !str_is_num((*value1).value, 0, strlen((*value1).value))
        ){
            error("trying to operate arithmetically with a non-numerical value.");
        }
        result = tan(atof((*value1).value));
        num_to_str(result_s, result);
        delete_element(value1);
        stack_push(result_s, 0, 50, true, false);
    }
    /* log */
    else if(comp_substr(source, token_start, token_end, "log")){
        stack_element * value1 = stack_pop();
        pnumber result;
        char result_s[50];
        if(
            !str_is_num((*value1).value, 0, strlen((*value1).value))
        ){
            error("trying to operate arithmetically with a non-numerical value.");
        }
        result = log(atof((*value1).value));
        num_to_str(result_s, result);
        delete_element(value1);
        stack_push(result_s, 0, 50, true, false);
    }
    /* = */
    else if(comp_substr(source, token_start, token_end, "=")){
        stack_element * value2 = stack_pop();
        stack_element * value1 = stack_pop();
        if(
            str_is_num((*value1).value, 0, strlen((*value1).value))
            && str_is_num((*value2).value, 0, strlen((*value2).value))
        ){
            pnumber val1 = atof((*value1).value);
            pnumber val2 = atof((*value2).value);
            stack_push(fabs(val1 - val2) < EPSILON ? "1" : "0", 0, 50, true, false);
        }else{
            if(strcmp((*value1).value, (*value2).value) == 0){
                stack_push("1", 0, 50, true, false);
            }else{
                stack_push("0", 0, 50, true, false);
            }
        }
        delete_element(value2);
        delete_element(value1);
    }
    /* != */
    else if(comp_substr(source, token_start, token_end, "!=")){
        stack_element * value2 = stack_pop();
        stack_element * value1 = stack_pop();
        if(
            str_is_num((*value1).value, 0, strlen((*value1).value))
            && str_is_num((*value2).value, 0, strlen((*value2).value))
        ){
            pnumber val1 = atof((*value1).value);
            pnumber val2 = atof((*value2).value);
            stack_push(fabs(val1 - val2) > EPSILON ? "1" : "0", 0, 50, true, false);
        }else{
            if(strcmp((*value1).value, (*value2).value) != 0){
                stack_push("1", 0, 50, true, false);
            }else{
                stack_push("0", 0, 50, true, false);
            }
        }
        delete_element(value2);
        delete_element(value1);
    }
    /* ! */
    else if(comp_substr(source, token_start, token_end, "!")){
        stack_element * value1 = stack_pop();
        if(strcmp((*value1).value, "0") == 0){
            stack_push("1", 0, 50, true, false);
        }else{
            stack_push("0", 0, 50, true, false);
        }
        delete_element(value1);
    }
    /* < */
    else if(comp_substr(source, token_start, token_end, "<")){
        stack_element * value2 = stack_pop();
        stack_element * value1 = stack_pop();
        if(
            str_is_num((*value1).value, 0, strlen((*value1).value))
            && str_is_num((*value2).value, 0, strlen((*value2).value))
        ){
            pnumber val1 = atof((*value1).value);
            pnumber val2 = atof((*value2).value);
            stack_push(val1 < val2 ? "1" : "0", 0, 50, true, false);
        }else{
            if(strcmp((*value1).value, (*value2).value) < 0){
                stack_push("1", 0, 50, true, false);
            }else{
                stack_push("0", 0, 50, true, false);
            }
        }
        delete_element(value2);
        delete_element(value1);
    }
    /* > */
    else if(comp_substr(source, token_start, token_end, ">")){
        stack_element * value2 = stack_pop();
        stack_element * value1 = stack_pop();
        if(
            str_is_num((*value1).value, 0, strlen((*value1).value))
            && str_is_num((*value2).value, 0, strlen((*value2).value))
        ){
            pnumber val1 = atof((*value1).value);
            pnumber val2 = atof((*value2).value);
            stack_push(val1 > val2 ? "1" : "0", 0, 50, true, false);
        }else{
            if(strcmp((*value1).value, (*value2).value) > 0){
                stack_push("1", 0, 50, true, false);
            }else{
                stack_push("0", 0, 50, true, false);
            }
        }
        delete_element(value2);
        delete_element(value1);
    }
    /* <= */
    else if(comp_substr(source, token_start, token_end, "<=")){
        stack_element * value2 = stack_pop();
        stack_element * value1 = stack_pop();
        if(
            str_is_num((*value1).value, 0, strlen((*value1).value))
            && str_is_num((*value2).value, 0, strlen((*value2).value))
        ){
            pnumber val1 = atof((*value1).value);
            pnumber val2 = atof((*value2).value);
            stack_push(val1 <= val2 ? "1" : "0", 0, 50, true, false);
        }else{
            if(strcmp((*value1).value, (*value2).value) <= 0){
                stack_push("1", 0, 50, true, false);
            }else{
                stack_push("0", 0, 50, true, false);
            }
        }
        delete_element(value2);
        delete_element(value1);
    }
    /* >= */
    else if(comp_substr(source, token_start, token_end, ">=")){
        stack_element * value2 = stack_pop();
        stack_element * value1 = stack_pop();
        if(
            str_is_num((*value1).value, 0, strlen((*value1).value))
            && str_is_num((*value2).value, 0, strlen((*value2).value))
        ){
            pnumber val1 = atof((*value1).value);
            pnumber val2 = atof((*value2).value);
            stack_push(val1 >= val2 ? "1" : "0", 0, 50, true, false);
        }else{
            if(strcmp((*value1).value, (*value2).value) >= 0){
                stack_push("1", 0, 50, true, false);
            }else{
                stack_push("0", 0, 50, true, false);
            }
        }
        delete_element(value2);
        delete_element(value1);
    }
    /* & */
    else if(comp_substr(source, token_start, token_end, "&")){
        stack_element * value2 = stack_pop();
        stack_element * value1 = stack_pop();
        if(strcmp((*value1).value, "0") != 0 && strcmp((*value2).value, "0") != 0){
            stack_push("1", 0, 50, true, false);
        }else{
            stack_push("0", 0, 50, true, false);
        }
        delete_element(value2);
        delete_element(value1);
    }
    /* | */
    else if(comp_substr(source, token_start, token_end, "|")){
        stack_element * value2 = stack_pop();
        stack_element * value1 = stack_pop();
        if(strcmp((*value1).value, "0") != 0 || strcmp((*value2).value, "0") != 0){
            stack_push("1", 0, 50, true, false);
        }else{
            stack_push("0", 0, 50, true, false);
        }
        delete_element(value2);
        delete_element(value1);
    }
    /* eval */
    else if(comp_substr(source, token_start, token_end, "eval")){
        stack_element * value = stack_pop();
        eval((*value).value);
        delete_element(value);
    }
    /* set */
    else if(comp_substr(source, token_start, token_end, "set")){
        stack_element * var = stack_pop();
        stack_element * value = stack_pop();
        set_var_value((*var).value, (*value).value);
        delete_element(var);
        delete_element(value);
    }
    /* get */
    else if(comp_substr(source, token_start, token_end, "get")){
        stack_element * var = stack_pop();
        get_var_value((*var).value);
        delete_element(var);
    }
    /* if */
    else if(comp_substr(source, token_start, token_end, "if")){
        stack_element * else_block = stack_pop();
        stack_element * if_block = stack_pop();
        stack_element * condition_block = stack_pop();
        stack_element * result;
        eval((*condition_block).value);
        result = stack_pop();
        if(strcmp((*result).value, "0") != 0){
            eval((*if_block).value);
        }else{
            eval((*else_block).value);
        }
        delete_element(else_block);
        delete_element(if_block);
        delete_element(condition_block);
        delete_element(result);
    }
    /* while */
    else if(comp_substr(source, token_start, token_end, "while")){
        stack_element * while_block = stack_pop();
        stack_element * condition_block = stack_pop();
        while(true){
            stack_element * result;
            eval((*condition_block).value);
            result = stack_pop();
            if(strcmp((*result).value, "0") != 0){
                delete_element(result);
                eval((*while_block).value);
            }else{
                delete_element(result);
                break;
            }
        }
        delete_element(while_block);
        delete_element(condition_block);
    }
    /* join */
    else if(comp_substr(source, token_start, token_end, "join")){
        stack_element * value2 = stack_pop();
        stack_element * value1 = stack_pop();
        size_t new_length = strlen((*value1).value) + strlen((*value2).value) + 1;
        char * new_value = malloc(sizeof(char) * new_length);
        strcpy(new_value, (*value1).value);
        strcat(new_value, (*value2).value);
        stack_push(new_value, 0, strlen(new_value), false, true);
        free(new_value);
        delete_element(value2);
        delete_element(value1);
    }
    /* copy */
    else if(comp_substr(source, token_start, token_end, "copy")){
        stack_element * value = stack_pop();
        stack_push((*value).value, 0, strlen((*value).value), false, true);
        stack_push((*value).value, 0, strlen((*value).value), false, true);
        delete_element(value);
    }
    /* del */
    else if(comp_substr(source, token_start, token_end, "del")){
        stack_element * value = stack_pop();
        delete_element(value);
    }
    /* swap */
    else if(comp_substr(source, token_start, token_end, "swap")){
        stack_element * value2 = stack_pop();
        stack_element * value1 = stack_pop();
        stack_push((*value2).value, 0, strlen((*value2).value), false, true);
        stack_push((*value1).value, 0, strlen((*value1).value), false, true);
        delete_element(value1);
        delete_element(value2);
    }
    /* input */
    else if(comp_substr(source, token_start, token_end, "input")){
        char * input = malloc(sizeof(char) * (MAXINPUTLENGTH + 1));
        fgets(input, MAXINPUTLENGTH, stdin);
        stack_push(input, 0, strlen(input), false, true);
        free(input);
    }
    /* >var */
    else if(comp_substr(source, token_start, token_start + 1, ">")){
        stack_element * value = stack_pop();
        char * var_name = malloc((token_end - token_start) * sizeof(char));
        copy_substr(var_name, source, token_start+1, token_end);
        set_var_value(var_name, (*value).value);
        free(var_name);
        delete_element(value);
    }
    /* @var */
    else if(comp_substr(source, token_start, token_start + 1, "@")){
        char * var_name = malloc((token_end - token_start) * sizeof(char));
        copy_substr(var_name, source, token_start+1, token_end);
        get_var_value(var_name);
        free(var_name);
    }
    /* % */
    else if(comp_substr(source, token_end-1, token_end, "%")){
        stack_element * value;
        char * var_name = malloc((token_end - token_start) * sizeof(char));
        copy_substr(var_name, source, token_start, token_end-1);
        get_var_value(var_name);
        free(var_name);
        value = stack_pop();
        eval((*value).value);
        delete_element(value);
    }
    /* random */
    else if(comp_substr(source, token_start, token_end, "random")){
        char result_s[50];
        num_to_str(result_s, ((double) rand() / (RAND_MAX)));
        stack_push(result_s, 0, 50, true, false);
    }
    /* exit */
    else if(comp_substr(source, token_start, token_end, "exit")){
        return 1;
    }
    /* Number */
    else if(str_is_num(source, token_start, token_end)){
        char result_s[50];
        copy_substr(result_s, source, token_start, token_end);
        num_to_str(result_s, atof(result_s));
        stack_push(result_s, 0, 50, true, false);
    }
    /* Plain strings */
    else{
        stack_push(source, token_start, token_end, true, false);
    }
    return 0;
}

void set_var_value(char* var, char* value){
    var_element * current_var;
    size_t string_length;
    current_var = variables;
    while(current_var != null){
        if(strcmp((*current_var).name, var) == 0){
            break;
        }
        else current_var = (*current_var).next;
    }
    if(current_var == null){
        size_t name_length;
        current_var = malloc(sizeof(var_element));
        name_length = strlen(var)+1;
        (*current_var).next = variables;
        (*current_var).name = malloc(sizeof(char) * name_length);
        strcpy((*current_var).name, var);
        variables = current_var;
    }else{
        free((*current_var).value);
    }
    string_length = strlen(value)+1;
    (*current_var).value = malloc(sizeof(char) * string_length);
    strcpy((*current_var).value, value);
}

void get_var_value(char* var){
    var_element * current_var = variables;
    while(current_var != null){
        if(strcmp((*current_var).name, var) == 0){
            stack_push((*current_var).value, 0, strlen((*current_var).value), true, true);
            return;
        }
        else current_var = (*current_var).next;
    }
    printf("When trying to get variable: %s\r\n", var);
    error("variable not found.");
}

void num_to_str(char* destination, pnumber number){
    size_t number_len;
    size_t i;
    char buffer[50];
    sprintf(buffer, "%f", number);
    number_len = strlen(buffer);
    for(i = number_len - 1; i > 0; --i){
        if(buffer[i] == '0'){
            buffer[i] = '\0';
        }
        else if(buffer[i] == '.' && i < number_len - 1 && buffer[i+1] == '\0'){
            buffer[i] = '\0';
            break;
        }
        else{
            break;
        }
    }
    strcpy(destination, buffer);
}

bool comp_substr(char* source, size_t from, size_t to, char* compare_to)
{
    size_t i;
    if(to - from != strlen(compare_to)) return false;
    /* Compare */
    for(i = from; i < to; ++i){
        if(source[i] != compare_to[i - from]){
            return false;
        }
    }
    return true;
}

void print_substr(char* source, size_t from, size_t to, bool trim)
{
    size_t i;
    if(trim){
        /* Find real start (trim) */
        for(i = from; i < to; ++i){
            if(source[i] == ' ' || source[i] == '\n' || source[i] == '\t'){
                from++;
            }else{
                break;
            }
        }
        /* Find real end (trim) */
        for(i = to-1; i >= from; --i){
            if(source[i] == ' ' || source[i] == '\n' || source[i] == '\t'){
                to--;
            }else{
                break;
            }
        }
    }
    /* Print */
    if(from < to){
        for(i = from; i < to; ++i){
            printf("%c", source[i]);
        }
        printf("\r\n");
    }
}

void copy_substr(char* destination, char* source, size_t from, size_t to){
    size_t dest_i = 0;
    size_t i;
    for(i = from; i < to; ++i){
        if(source[i] == '\r') continue;
        else if(source[i] == '\n') destination[dest_i] = ' ';
        else destination[dest_i] = source[i];
        ++dest_i;
    }
    destination[dest_i] = '\0';
}

void stack_push(char* source, size_t from, size_t to, bool trim, bool pushempty){
    size_t i;
    if(trim){
        for(i = from; i < to; ++i){
            if(source[i] == ' ' || source[i] == '\n' || source[i] == '\t'){
                from++;
            }else{
                break;
            }
        }
        for(i = to-1; i >= from; --i){
            if(source[i] == ' ' || source[i] == '\n' || source[i] == '\t'){
                to--;
            }else{
                break;
            }
        }
    }
    if(from < to || pushempty){
        stack_element * element_to_push;
        size_t string_length = to - from;
        element_to_push = malloc(sizeof(stack_element));
        (*element_to_push).value = malloc(sizeof(char) * (string_length + 1));
        copy_substr((*element_to_push).value, source, from, to);
        (*element_to_push).next = stack;
        stack = element_to_push;
        if(show_pushpops) printf("Push: %s\r\n", (*stack).value);
    }
}

stack_element * stack_pop(){
    stack_element * popped = stack;
    if(stack == null){
        error("cannot pop from an empty stack.");
    }
    if(show_pushpops) printf("Pop: %s\r\n", (*popped).value);
    stack = (*stack).next;
    return popped;
}

void delete_element(stack_element * se){
    free((*se).value);
    free(se);
}
