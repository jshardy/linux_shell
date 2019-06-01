/*************************************************************
* Professor:    Jesse Chaney
* Author:		Joshua Hardy
* Filename:		owlsh.c
* Date Created:	5/17/2017
* Date Modified: 12/23/2017 - Finished project
*   passes valgrind
**************************************************************/
/*************************************************************
* Lab/Assignment: Lab6 - 7 - Stage 2
*
* Overview:
*   Custom shell, implementing:
*   exit, cd, cd <dir>, pwd, echo
* Input:
*	from command line : stdin
* Output:
*   from commands and external commands : stdout
* Usage:
*   owlsh user-name>>
*   ls -l > file1.txt
*   who -H > file2.txt
*   wc -lw < file1.txt
*   wc -c < file1.txt > file2.txt
*   OR
*   ls -l >file1.txt
*   who -H>file2.txt
*   wc -lw<file1.txt
*   wc -c<file1.txt > file2.txt
****ADDITIONS****
*   ls -la -F | wc -l
*   ps -elf | grep http
*   ps -elf | grep jesse | tr 'j' 'm'
*   ps -elf | grep jesse | tr 'j' 'm' | awk {printf"%s\n", $3);} > file1.text
*****BOTH WILL WORK, NO STRTOK'EN!!!************************
************************************************************/
#include <stdio.h>      //stdc
#include <stdlib.h>     //qsort
#include <stdarg.h>     //va_list
#include <unistd.h>     //getcwd pid_t
#include <sys/types.h>  //open
#include <sys/stat.h>   //open
#include <fcntl.h>      //open
#include <string.h>
#include <ctype.h>
#include <linux/limits.h>   //for PATH_MAX
#include <sys/wait.h>   //wait()
#include <errno.h>     //errno

#include "owlsh.h"

//globals
BOOL verbose_mode;
BOOL end_of_world;

int main(int argc, char **argv)
{
    char prompt[PROMPT_SIZE] = { 0 };
    char *user_name = NULL;
    char *bad_user = "$LOGNAME";
    char c = 0;
    char *str_line = NULL;

    //init globals
    verbose_mode = FALSE;
    end_of_world = FALSE;

    //mainly for setting verbose mode and displaying help
    while ((c = getopt(argc, argv, "hv")) != -1) {
      switch (c)
      {
          case 'h':
            fprintf(stdout, "To see built in commands type help\n");
            break;
          case 'v':
            verbose_mode = TRUE;
            break;
      }
    }
    //set the prompt string
    if((user_name = getenv("LOGNAME")) == NULL)
    {
        user_name = bad_user;
        fprintf(stderr, COLOR_RED "Error: LOGNAME not set in environment\n" COLOR_RESET);
    }
    sprintf(prompt, PROMPT_STR " %s>> ", user_name);

    //begining of shell prompt
    do
    {
        //output prompt
        fprintf(stdout, "%s", prompt);
        //get commandline
        str_line = shell_get_line(stdin);

        if(str_line != NULL)
        {
            cmd_t *head = parse_command(str_line);

            //execute command
            execute_command(head);

            //do cleanup before next command
            free(str_line);
            str_line = NULL;
            free_command(head);
            head = NULL;
        }
    } while(end_of_world == FALSE);

    return 0;
}

/**********************************************************************
* Function:         char *shell_get_line(FILE *input)
* Purpose:          get a line from FILE *input
* Precondition:     pass FILE *input
* Postcondition:    returns a string of FILE *input.
*                   *****YOU MUST FREE STRING*****
*                   Returns string of input or NULL if no value.
************************************************************************/
char *shell_get_line(FILE *input)
{
    size_t current_line_size = MAX_STR;
    char *str_line = NULL;
    char c = 0;
    size_t index = 0;
    str_line = malloc(sizeof(char) * current_line_size);

    while((c = fgetc(input)) != '\n')
    {
        str_line[index++] = c;
        //line too long? reallocated space for it
        if(index == current_line_size)
        {
            current_line_size += MAX_STR;
            str_line = realloc(str_line, sizeof(char) * current_line_size);
            if(str_line == NULL)
            {
                fprintf(stderr, COLOR_RED "Out of memory!" COLOR_RESET);
                shell_soft_exit();
                //jump out of inner-loop and return control to main
                //so we can close softly
                break;
            }
        }
    }
    if(str_line != NULL)
    {
        //add null terminator and trim whitespace from b b both ends
        //and remove control characters
        str_line[index] = 0;
        remove_control_characters(str_line);
        trim_string(str_line);
    }

    if(verbose_mode)
        fprintf(stderr, COLOR_RED "str_line: %s\n" COLOR_RESET, str_line);

    return str_line;
}

/**********************************************************************
* Function:         int execute_command(cmd_t *commands)
* Purpose:          execute external command via fork() and execvp()
* Precondition:     pass cmd_t *
* Postcondition:    Returns true(1) if command executed properly.
*                   Returns False if failed.
************************************************************************/
int execute_command(cmd_t *commands)
{
    //THIS NEEDS TO BE CLEANED UP... USE A SWITCH OR SOMETHING!
    cmd_t *head = commands;
    cmd_t *travel = head;
    cmd_t *tail = NULL;
    pid_t *pid = NULL;
    int fd[2] = { 0 };
    int old_input = 0;
    int process_count = 0;
    int r_val = 0;
    int i = 0;
    //get process count and tail
    while(travel != NULL)
    {
        process_count++;
        tail = travel;
        travel = travel->next;
    }
    //reset travel
    travel = head;
    //allocate pid's and zero them
    pid = malloc(sizeof(pid_t) * process_count);
    memset(pid, 0, sizeof(pid_t) * process_count);

    if(is_internal_command(head))
    {
        execute_internal_command(head);
    }
    else
    {
        //single command
        if(process_count == 1)
        {
            //input and output redirect
            if((head->input_src == REDIRECT_FILE) && (head->output_dest == REDIRECT_FILE))
            {
                create_process_file_redirects(head, head->input_file_name, head->output_file_name, -1, -1);
            }
            //only input redirect
            else if(head->input_src == REDIRECT_FILE)
            {
                create_process_file_redirects(head, head->input_file_name, NULL, -1, STDOUT_FILENO);
            }
            //only output redirect
            else if(head->output_dest == REDIRECT_FILE)
            {
                create_process_file_redirects(head, NULL, head->output_file_name, STDIN_FILENO, -1);
            }
            //no redirects execute it.
            else
            {
                create_process(head, STDIN_FILENO, STDOUT_FILENO);
            }
        }
        //Multi-command pipes
        else if(process_count > 1)
        {
            //capture the input redirect for first command
            if(head->input_src == REDIRECT_FILE)
            {
                pipe(fd);
                pid[0] = create_process_file_redirects(head, head->input_file_name, NULL, -1, fd[1]);
                close(fd[1]);
                old_input = fd[0];
                //move travel one forward
                travel = travel->next;
                i = 1;
            }
            //else i = 0; //set up top

            for(; (i < process_count - 1) && (travel != tail); i++)
            {
                pipe(fd);   //do error check on this?
                pid[i] = create_process(travel, old_input, fd[1]);
                close(fd[1]);
                old_input = fd[0];
                travel = travel->next;
            }

            if(tail->output_dest == REDIRECT_FILE)
            {
                pid[process_count - 1] = create_process_file_redirects(tail, NULL, tail->output_file_name, old_input, -1);
            }
            else
            {
                pid[process_count - 1] = create_process(tail, old_input, STDOUT_FILENO);
            };
            close(fd[0]);
        }

        travel = head;
        while(travel != NULL)
        {
            if(is_internal_command(travel) == FALSE)
            {
                wait(&r_val);
                fprintf(stderr, COLOR_RED "%s: exit value %i\n" COLOR_RESET, travel->cmd, r_val);
            }
            travel = travel->next;
        }
    }
    free(pid);
    //for now return true, but in future make checks to make sure command executed.
    //ie pid == -1
    return TRUE;
}
//if you pass in int inputs or outputs it will use them instead. this allows for pipes.
pid_t create_process_file_redirects(cmd_t *cmd, char *input_file_name, char *output_file_name, int file_input, int file_output)
{
    pid_t pid = -1;
    char **array_args = NULL;

    pid = fork();
    if(pid == 0)
    {
        //child
        if(input_file_name != NULL)
        {
            file_input = open(input_file_name, O_RDONLY);
            //if failed to open kill child process
            if(file_input == -1)
            {
                fprintf(stderr, COLOR_RED "%s: input redirect couldn't open. Terminating child.\n" COLOR_RESET, input_file_name);
                //perror("Bad command or filename\n");
                exit(EXIT_FAILURE);
            }
        }
        if(output_file_name != NULL)
        {
            file_output = open(output_file_name, O_CREAT | O_TRUNC | O_WRONLY);
            //if failed to open kill child process
            if(file_output == -1)
            {
                fprintf(stderr, COLOR_RED "%s: output redirect couldn't open. Terminating child.\n" COLOR_RESET, output_file_name);
                exit(EXIT_FAILURE);
            }
        }
        quick_dup(file_input, file_output);
        //since the child resources get destroyed I don't have to worry about free
        array_args = get_args(cmd->cmd, cmd->raw_params);

        execvp(cmd->cmd, array_args);
        //should never get executed.
        free_args(array_args);
        //if execvp fails output error and kill child.
        fprintf(stderr, COLOR_RED "%s: Bad command or filename?\n" COLOR_RESET, cmd->cmd);
        //perror("Bad command or filename\n");
        exit(EXIT_FAILURE);
    }
    //parent
    return pid;
}

pid_t create_process(cmd_t *cmd, int file_input, int file_output)
{
    pid_t pid = -1;

    char **array_args = NULL;
    pid = fork();

    if(pid == 0)
    {
        //child
        quick_dup(file_input, file_output);
        //since the child resources get destroyed I don't have to worry about free
        array_args = get_args(cmd->cmd, cmd->raw_params);

        execvp(cmd->cmd, array_args);
        //should never get executed.
        free_args(array_args);
        //if execvp fails output error and kill child.
        fprintf(stderr, COLOR_RED "%s: Bad command or filename?\n" COLOR_RESET, cmd->cmd);
        //perror("Bad command or filename\n");
        exit(EXIT_FAILURE);
    }
    //parent
    return pid;
}

void quick_dup(int file_input, int file_output)
{
    //avoid closing 0 for first input
    if(file_input != STDIN_FILENO)
    {
        dup2(file_input, STDIN_FILENO);
        close(file_input);
    }
    //avoid closing output for last output
    if(file_output != STDOUT_FILENO)
    {
        dup2(file_output, STDOUT_FILENO);
        close(file_output);
    }
}

int execute_internal_command_redirect_from_file(cmd_t *command, char *file_input_name, char *file_output_name)
{
    int file_input = 0;
    int file_output = 0;
    int r_val = 0;
    if(file_input_name != NULL)
        file_input = open(file_input_name, O_RDONLY);
    if(file_output_name != NULL)
        file_output = open(file_output_name, O_WRONLY | O_TRUNC | O_CREAT);

    if((file_input != -1) && (file_output != -1))
    {
        r_val = execute_internal_command_redirect(command, file_input, file_output);
        close(file_input);
        close(file_output);
    }
    else if(file_input != -1)
    {
        r_val = execute_internal_command_redirect(command, file_input, STDOUT_FILENO);
        close(file_input);
    }
    else if(file_output != -1)
    {
        r_val = execute_internal_command_redirect(command, STDIN_FILENO, file_output);
        close(file_output);
    }
    else
        fprintf(stderr, COLOR_RED "Failed to open file redirect on internal command.\n" COLOR_RESET);

    return r_val;
}

int execute_internal_command_redirect(cmd_t *command, int file_input, int file_output)
{
    int old_input = 0;
    int old_output = 0;
    int r_val = 0;

    //make copies of file STDs
    old_input = dup(STDIN_FILENO);
    old_output = dup(STDOUT_FILENO);

    //closes STDIN/STDOUT for me
    dup2(file_input, STDIN_FILENO);
    dup2(file_output, STDOUT_FILENO);
    r_val = execute_command(command);
    dup2(old_input, STDIN_FILENO);
    dup2(old_output, STDOUT_FILENO);

    return r_val;
}

/**********************************************************************
* Function:         int execute_internal_command(cmd_t *command)
* Purpose:          pass cmd_t structure and will test for execute
*                   of internal commands.
* Precondition:     pass cmd_t *
* Postcondition:    Returns true(1) if internal command.
*                   Returns False if external(0).
************************************************************************/
int execute_internal_command(cmd_t *command)
{
    int is_command = FALSE;
    int r_val = 0;
    char current_dir[PATH_MAX] = { 0 };

    if((is_command = is_internal_command(command)) > 0)
    {
        switch(is_command)
        {
            case 1: //cd
                //if cd has no args goto home folder of user.
                if(command->raw_params == NULL)
                    r_val = chdir(getenv("HOME"));
                else
                    r_val = chdir(command->raw_params);

                if(r_val == -1)
                    fprintf(stderr, COLOR_RED "Couldn't change directory\n" COLOR_RESET);
                break;
            case 2: //pwd
                getcwd(current_dir, PATH_MAX);
                if(current_dir != NULL)
                    fprintf(stdout, "%s\n", current_dir);
                break;
            case 3: //echo
                fork();
                fprintf(stdout, "%s\n", command->raw_params);
                break;
            case 4: //help
                display_shell_help();
                break;
            case 5: //exit
                shell_soft_exit();
                break;
            case 6: //verbose
                verbose_mode = TRUE;
                break;
        }
    }

    return is_command;
}

int is_internal_command(cmd_t *command)
{
    int is_command = FALSE;
    //check for internal command
    if(strcmp(command->cmd, "cd") == 0)
        is_command = 1;
    else if(strcmp(command->cmd, "pwd") == 0)
        is_command = 2;
    else if(strcmp(command->cmd, "echo") == 0)
        is_command = 3;
    else if(strcmp(command->cmd, "help") == 0)
        is_command = 4;
    else if((strcmp(command->cmd, "exit") == 0) || (strcmp(command->cmd, "quit") == 0))
        is_command = 5;
    else if(strcmp(command->cmd, "verbose") == 0)
        verbose_mode = 6;

    return is_command;
}

/**********************************************************************
* Function:         char **get_args(char *cmd, char *args)
* Purpose:          parses arguments into ** list of strings null terminated
* Precondition:     pass *cmd for arg[0] and *args to be created into ->
* Postcondition:    list of strings returned for exec functions
*                   *****YOU MUST FREE ARGS VIA free_args(**args)******
************************************************************************/
char **get_args(char *cmd, char *args)
{
    char *p = NULL;
    char **arg = NULL;
    size_t arg_count = 1;

    if((cmd != NULL) && (args != NULL))
    {
        //throw the name of command as first arg
        arg = malloc(sizeof(char*));
        //forgot to put parentheses around (strlen(cmd) + 1) cost of 2 hours...
        //face palm. Not the first time I've done this exact same error.
        //Sadly the order of operations makes since.
        arg[0] = strdup(cmd);

        //get the rest of the arguments into the array
        p = strtok(args, " ");
        while(p != NULL)
        {
            arg = realloc(arg, sizeof(char*) * (arg_count + 1));
            arg[arg_count] = strdup(p);
            arg_count++;

            p = strtok(NULL, " ");
        }
        //null terminate the list of c_strings
        arg = realloc(arg, sizeof(char*) * (arg_count + 1));
        arg[arg_count] = NULL;
    }
    else
    {
        //this fixes strange error of getting control char's as arguments for
        //subsequent commands... example ls -al then ls. Was causing
        //control characters to be read in for arguments....
        arg = malloc(sizeof(char*) * 2);
        arg[0] = strdup(cmd);
        arg[1] = NULL;
    }
    if(verbose_mode)
        display_args(arg);

    return arg;
}

/**********************************************************************
* Function:         void display_args(char **args)
* Purpose:          displays a ** list of strings null terminated
* Precondition:     pass ** list of strings
* Postcondition:    list of strings displayed
************************************************************************/
void display_args(char **args)
{
    fprintf(stderr, COLOR_RED "Argument Array\n" COLOR_RESET);
    while(*args != NULL)
    {
        fprintf(stderr, COLOR_RED "%s\n" COLOR_RESET, *args);
        args++;
    }
}

/**********************************************************************
* Function:         void free_args(char **args)
* Purpose:          free **pointer that is null terminated (strings)
* Precondition:     pass ** list of strings
* Postcondition:    list of strings free'd
************************************************************************/
void free_args(char **args)
{
    char **args_save = args;
    if(args != NULL)
    {
        while(*args != NULL)
        {
            //free strings in list
            free(*args);
            *args = NULL;
            args++;
        }
        //free the list
        free(args_save);
    }
    //didn't use tripple star so can't change address of **args to NULL.
}

/**********************************************************************
* Function:         cmd_t *parse_command(const char *cmd)
* Purpose:          intakes a string and converts it to cmd_t (lex/parser)
* Precondition:     pass pointer to valid command string
* Postcondition:    command string parsed and returned as a cmd_t
* Comments:         This is a terrible function....break it into smaller
                    inline functions...yuk
************************************************************************/
cmd_t *parse_command(const char *cmd)
{
    char *str_begin = NULL;
    char *str_end = NULL;
    cmd_t *head = NULL;
    cmd_t *commands = NULL;
    BOOL found_redirect = FALSE;
    BOOL found_pipe = FALSE;

    if(*cmd != 0)
    {
        head = commands;
        do
        {
            //reset it incase this is second interation
            found_pipe = FALSE;
            if(head == NULL)
            {
                head = commands = malloc(sizeof(cmd_t));
                //set everything to zero.
                memset(commands, 0, sizeof(cmd_t));
                trim_string((commands->raw_cmd = strdup(cmd)));
            }
            else
            {
                //could go in redirect loop, but I like it closer to the above code.
                commands->raw_cmd = strdup(str_end);
            }

            //get command
            str_begin = str_end = commands->raw_cmd;
            while((*str_end != '>') && (*str_end != '<') && (*str_end != ' ') && (*str_end != '|') && (*str_end != 0))
                str_end++;

            commands->cmd = strndup(str_begin, str_end - str_begin);
            str_begin = str_end;
            //end get command

            //begin finding arguments
            while((*str_end != 0) && (*str_end != '>') && (*str_end != '<') && (*str_end != '|'))
                str_end++;

            if(str_end != str_begin)
            {
                commands->raw_params = strndup(str_begin + 1, (str_end - str_begin) - 1);
                trim_string(commands->raw_params);
                str_begin = str_end;
            }
            //end arguments

            //find file redirect
            while(*str_end != 0)
            {
                //pipe
                if(*str_end == '|')
                {
                    commands->output_dest = REDIRECT_PIPE;
                    //walk forward past |
                    str_end++;
                    //skip leading whitespace.
                    while(*str_end == ' ')
                        str_end++;

                    commands->next = malloc(sizeof(cmd_t));
                    //move forward
                    commands = commands->next;
                    //zero new struct
                    memset(commands, 0, sizeof(cmd_t));
                    commands->input_src = REDIRECT_PIPE;

                    found_pipe = TRUE;
                    break;
                }
                //output
                if(*str_end == '>')
                {
                    //bump it forward once so we can pickup future >
                    str_end++;
                    //get rid of leading spaces
                    while(*str_end == ' ')
                        str_end++;
                    str_begin = str_end;
                    //find the end
                    while((*str_end != 0) && (*str_end != ' ') && (*str_end != '<') && (*str_end != '>'))
                        str_end++;
                    //if we didn't move then do nothing
                    if((str_end != str_begin) && (commands->output_file_name == NULL))
                    {
                        commands->output_dest = REDIRECT_FILE;
                        //copy +1 to skip >
                        commands->output_file_name = strndup(str_begin, str_end - str_begin);
                        //trim_string(commands->output_file_name);
                        //set new start point for other if's.
                        str_begin = str_end;
                    }
                    //don't need this here because, next "if" comes immediately after
                    //found_redirect = TRUE;
                }
                //input
                if(*str_end == '<')
                {
                    //bump it forward once so we can pickup future <
                    str_end++;
                    //get rid of leading spaces
                    while(*str_end == ' ')
                        str_end++;
                    str_begin = str_end;
                    //find the end
                    while((*str_end != 0) && (*str_end != ' ') && (*str_end != '<') && (*str_end != '>'))
                        str_end++;
                    //if we didn't move then do nothing
                    if((str_end != str_begin) && (commands->input_file_name == NULL))
                    {
                        commands->input_src = REDIRECT_FILE;
                        //copy +1 to skip <
                        commands->input_file_name = strndup(str_begin, str_end - str_begin);
                        //trim_string(commands->input_file_name);
                        //set new start point for other if's.
                        str_begin = str_end;
                    }
                    //needed here because next "if" isn't for one whole iteration.
                    //this allows iteration to not skip a character
                    found_redirect = TRUE;
                }
                //using this value so we don't miss opposing redirects(<>)
                if(found_redirect == TRUE)
                    found_redirect = FALSE;
                else
                    str_end++;
            }
            //end file redirect
        } while(found_pipe);
    }

    if(verbose_mode)
        display_command_structure(head);

    return head;
}

/**********************************************************************
* Function:         void remove_control_characters(char *str_line)
*       implemented for good measure
* Purpose:          remove all control characters from a string
* Precondition:     pass a string
* Postcondition:    all control characters removed from string
************************************************************************/
void remove_control_characters(char *str_line)
{
    if(str_line != NULL)
    {
        while(*str_line != 0)
        {
            if(((*str_line >= 1) && (*str_line <= 31)) || (*str_line == 177))
                *str_line = ' ';
            str_line++;
        }
    }
}

/**********************************************************************
* Function:         void free_command(cmd_t *commands)
* Purpose:          free's cmd_t structure
* Precondition:     a cmd_t pointer
* Postcondition:    structure free'd
************************************************************************/
void free_command(cmd_t *commands)
{
    //doesn't modify pointer....no double star use
    cmd_t *travel = commands;
    cmd_t *trail = NULL;

    while(travel)
    {
        //all pointer inited to null
        //so free won't do anything on those
        free(travel->cmd);
        free(travel->raw_cmd);
        free(travel->raw_params);
        free(travel->input_file_name);
        free(travel->output_file_name);

        trail = travel;
        travel = travel->next;
        free(trail);
    }
}

/**********************************************************************
* Function:         void display_command_structure(cmd_t *command)
* Purpose:          for debug - displays display_command_structure
* Precondition:     a cmd_t pointer
* Postcondition:    structure displayed without delay(no buffer)
************************************************************************/
void display_command_structure(cmd_t *command)
{
    while(command != NULL)
    {
        fprintf(stderr, COLOR_RED "\nCommand Structure View\n");
        fprintf(stderr, "CMD: \"%s\"\n", command->cmd);
        fprintf(stderr, "RAW_CMD: \"%s\"\n", command->raw_cmd);
        fprintf(stderr, "ARGUMENTS: \"%s\"\n", command->raw_params);
        fprintf(stderr, "INPUT_FILE: \"%s\"\n", command->input_file_name);

        if(command->input_src != REDIRECT_PIPE)
            fprintf(stderr, "INPUT_REDIRECT: \"%s\"\n", command->input_src == REDIRECT_FILE ? "FILE" : "NONE");
        else
            fprintf(stderr, "INPUT_REDIRECT: \"PIPE\"\n");

        fprintf(stderr, "OUTPUT_FILE: \"%s\"\n", command->output_file_name);

        if(command->output_dest != REDIRECT_PIPE)
            fprintf(stderr, "OUTPUT_REDIRECT: \"%s\"\n", command->output_dest == REDIRECT_FILE ? "FILE" : "NONE");
        else
            fprintf(stderr, "OUTPUT_REDIRECT: \"PIPE\"\n");

        fprintf(stderr, "NEXT COMMAND? \"%s\"\n" COLOR_RESET, command->next == NULL ? "NO" : "YES");
        command = command->next;
    }
}

/* Belongs in its own file */
/**********************************************************************
* Function:         trim_string("  my string   ")
* Purpose:          trims spaces from front and back.
* Precondition:     a string pointer
* Postcondition:    string without leading or following spaces
************************************************************************/
void trim_string(char *string)
{
    left_trim(string);
    right_trim(string);
}

/* Belongs in its own file */
/**********************************************************************
* Function:         left_trim("   my string")
* Purpose:          trims spaces from front and back.
* Precondition:     a string pointer
* Postcondition:    string without leading spaces`
************************************************************************/
void right_trim(char *string)
{
    //find the end of string
    while(*string != 0)
        string++;

    //now were at the end. Backup one and see if space
    string--;
    while(*string == ' ')
        *(string--) = 0;
}
/* Belongs in its own file */
/**********************************************************************
* Function:         right_trim("my string   ")
* Purpose:          trims spaces from end.
* Precondition:     a string pointer
* Postcondition:    string without ending spaces.
************************************************************************/
void left_trim(char *string)
{
    char *begin = string;
    //front spaces removal
    while(*string == ' ')
        string++;

    //did we have spaces? copy over them
    if (string != begin)
        memmove(begin, string, strlen(string) + 1);
}

void display_shell_help(void)
{
    const char str_help[] = "Built-in Command | Action\n"
                     "exit - exit's owlshell\n"
                     "cd <dir> - change directory\n"
                     "pwd - display current directory\n"
                     "echo text_display - displays text on console\n"
                     "help - display internal command help <-this->\n";
    fprintf(stdout, str_help);
}

void shell_soft_exit(void)
{
    end_of_world = TRUE;
}
