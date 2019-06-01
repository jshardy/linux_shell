//robbed from professor Jesse Chaney
#ifndef OWLSH
#define OWLSH

#define MAX_STR 1024
#define TRUE    1
#define FALSE   0

#define PROMPT_STR  "owlsh"
#define PROMPT_SIZE 64

//taken from a random website
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

typedef unsigned char BOOL;

typedef enum {
  REDIRECT_NONE
  , REDIRECT_FILE
  , REDIRECT_PIPE
} redir_t;

typedef struct cmd_s
{
    char *cmd;
    char *raw_cmd;
    char *raw_params;
    char *input_file_name;
    char *output_file_name;
    redir_t input_src;
    redir_t output_dest;
    struct cmd_s *next;
} cmd_t;

char *shell_get_line(FILE *input);
int execute_command(cmd_t *commands);
int execute_internal_command(cmd_t *command);
int execute_internal_command_redirect(cmd_t *command, int file_input, int file_output);
int execute_internal_command_redirect_from_file(cmd_t *command, char *file_input_name, char *file_output_name);
cmd_t *parse_command(const char *cmd);
void free_command(cmd_t *commands);
void trim_string(char *string);
void right_trim(char *string);
void left_trim(char *string);
void display_commands(void);
void display_help(void);
void shell_output(const char *output_string, ...);
void display_command_structure(cmd_t *command);
void shell_soft_exit(void);
char **get_args(char *cmd, char *args);
void free_args(char **args);
void display_args(char **args);
void display_shell_help(void);
void remove_control_characters(char *str_line);
pid_t create_process(cmd_t *cmd, int file_input, int file_output);
pid_t create_process_file_redirects(cmd_t *cmd, char *input_file_name, char *output_file_name, int file_input, int file_output);
int is_internal_command(cmd_t *command);
void quick_dup(int file_input, int file_output);

#endif
