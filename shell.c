
/*
  Function Declarations for builtin shell commands:
 */

/*******************************************************************************/

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "shell.h"


#define TOK_DEL " \t\r\n\a"

/*******************************************************************************/
static int lsh_num_builtins_arg();

static int lsh_num_builtins_no_arg();

/*
 * static int lsh_cd(Cmd cmd);
 * apply command cd
 *
 * Parameters
 * cmd  a data structure containing arguments
*/

static int lsh_cd(Cmd cmd);

/*
 * static int lsh_help(Cmd cmd);
 * apply command help
 *
 * Parameters
 * cmd  a data structure containing arguments
*/

static int lsh_help(); 

/*
 * static int lsh_exit(Cmd cmd);
 * apply command exit
*/

static int lsh_exit();

/*
 * static int bash_launch_exec(Cmd cmd);
 * used to execute the given command
*/

static int bash_launch_exec(Cmd cmd);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str_no_arg[] = {
  "help",
  "exit"
};

char *builtin_str_arg[] = {
  "cd"
};

int (*builtin_func_no_arg[]) () = {
  &lsh_help,
  &lsh_exit
};

int (*builtin_func_arg[]) (Cmd cmd) = {
  &lsh_cd,
};

static int lsh_num_builtins_no_arg() {
  return sizeof(builtin_str_no_arg) / sizeof(char *);
}

static int lsh_num_builtins_arg() {
  return sizeof(builtin_str_arg) / sizeof(char *);
}

///////static function///////

static int lsh_cd(Cmd cmd){
    int returnvalue = 1;
    if (strcmp(cmd.tokens[1],"~") == 0 || cmd.n_arguments == 1){
        if (chdir(getenv("HOME")) != 0) {
            returnvalue = 0;
        }
    }
    else if ((strcmp(cmd.tokens[1],"..") == 0)){
        if (chdir("../") != 0) {
            returnvalue = 0;
        }
    }
    else {
        if (chdir(cmd.tokens[1]) != 0) {
            returnvalue = 0;
        }
    }
    return returnvalue;
}

static int lsh_help(){
    printf("Author : Theo Stassen, Ludovic Sangiovanni\n");
    fflush(stdout);
    return 1;
}

static int lsh_exit(){
    return -1;
}

static int status_to_return_value(int status){
    int returnvalue = 0;
    if(status == 0)
        returnvalue = 1;
    else
        returnvalue = 0;
    return returnvalue;
}

static int bash_launch_exec(Cmd cmd)
{
    pid_t pid, tpid;
    int pfd[MAX_CMD_SIZE][2];

    char *args[MAX_CMD_SIZE];        
    int i = 0, pipe_locate[MAX_CMD_SIZE], pipe_count = 0;
    pipe_locate[0] = -1;

    for(i=0; i < cmd.n_arguments; i++){
        args[i] = cmd.tokens[i];
        if(strcmp(cmd.tokens[i], "|") == 0){
            pipe_count++;
            pipe_locate[pipe_count] = i;
            args[i] = NULL;
        }
    }
    args[i] = NULL;

    if(pipe_count == 0){
        int status;
        pid = fork();
        if (pid == -1)
            return -1;
     
        if (pid == 0){
            execvp(args[0], args);
            printf("\n");
            fflush(stdout);
            exit(1);
        }
        else{
            do{
                tpid = wait(&status);
                if(tpid != pid)
                    exit(tpid);                
            } while(tpid != pid);

            return status_to_return_value(status);
        }
    }

    for (i = 0; i < pipe_count + 1 && pipe_count != 0; i++) {
        if (i != pipe_count) pipe(pfd[i]);

        if (fork() == 0) {
            if (i == 0) {
                dup2(pfd[i][1], 1);
                close(pfd[i][0]); 
                close(pfd[i][1]);
            } else if (i == pipe_count) {
                dup2(pfd[i - 1][0], 0);
                close(pfd[i - 1][0]); 
                close(pfd[i - 1][1]);
            } else {
                dup2(pfd[i - 1][0], 0);
                dup2(pfd[i][1], 1);
                close(pfd[i - 1][0]); close(pfd[i - 1][1]);
                close(pfd[i][0]); close(pfd[i][1]);
            }

            execvp(args[pipe_locate[i] + 1], args + pipe_locate[i] + 1);
            exit(1);
        }
        else if (i > 0) {
            close(pfd[i - 1][0]); close(pfd[i - 1][1]);
        }
    }

    int status;

    for (i = 0; i < pipe_count + 1; i++) {
            wait(&status);
    }
    return status_to_return_value(status);
}

////end of static functions

Cmd *create_cmd(){
    Cmd *cmd = malloc(sizeof(Cmd));
    if(cmd == NULL)
        return NULL;
    cmd->n_arguments = 0;

    return cmd;           
}

void free_cmd(Cmd *cmd){
    if(cmd == NULL)
        return;
    free(cmd);
}

int read_line(Cmd* cmd){
    char buffer[MAX_CMD_SIZE*MAX_CMD_SIZE+1];
    char *token;
    int position = 0;
    int c;

    //Loop to read the line
    int finish = 0;
    while (!finish) {


        //We read the next character
        c = getchar();


        if (c == EOF || c == '\n') 
        //In this case we reached the end of the line, we have our buffer
        {
            buffer[position] = '\0';
            finish = 1;
        } 
        //In this case we put the character in the buffer
        else 
        {
            buffer[position] = c;
        }

        position++;
    }

    position = 0;

    //Loop to parse the line
    token = strtok(buffer, TOK_DEL);
    int n_args = 0;
    while (token != NULL) {
        strcpy(cmd->tokens[position], token);
        position++;
        token = strtok(NULL, TOK_DEL);
        n_args++;
    }

    cmd->n_arguments = n_args;

    return 0;
}


int launch(Cmd cmd){
    int i;

    if (cmd.tokens[0] == NULL) {
        // An empty command was entered.
        return 1;
    }

    for (i = 0; i < lsh_num_builtins_no_arg(); i++) {
        if (strcmp(cmd.tokens[0], builtin_str_no_arg[i]) == 0) {
            return (*builtin_func_no_arg[i]) ();
        }
    }
    for (i = 0; i < lsh_num_builtins_arg(); i++) {
        if (strcmp(cmd.tokens[0], builtin_str_arg[i]) == 0) {
            return (*builtin_func_arg[i])(cmd);
        }
    }
    return bash_launch_exec(cmd);
}