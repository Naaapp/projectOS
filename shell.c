
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

	#define MAX_LINE_LENGHT 256
	#define COMMAND_NOT_FOUND_ERROR 0

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
	    if (strcmp(cmd.tokens[1],"~") == 0){
	        if (chdir(getenv("HOME")) != 0) {
	            perror("lsh"); returnvalue = 0;
	        }
	    }
	    else if ((strcmp(cmd.tokens[1],"..") == 0)){
	  	    if (chdir("../") != 0) {
	            perror("lsh"); returnvalue = 0;
	        }
	    }
	    else {
	        if (chdir(cmd.tokens[1]) != 0) {
	            perror("lsh"); returnvalue = 0;
	        }
	    }
	    return returnvalue;
	}

	static int lsh_help(){
	    printf("Author : Theo Stassen, Ludovic Sangiovanni\n");
	    return 1;
    }

	static int lsh_exit(){
	    return -1;
	}

    static int bash_launch_exec(Cmd cmd)
	{
		pid_t pid, wpid;
		int fd[2];
		int returnvalue =1;
		int status = 0;

		pipe(fd);

		pid = fork();
        
		if (pid == -1)
		{
			char* error = strerror(errno);
        	printf("fork error: %s\n", error);
        	return -1;
		}
		else if (pid == 0) // Child process
		{
			int exec_result = 0;
            if(cmd.n_arguments == 1){
                //Get the current directory
                char currDir[MAX_LINE_LENGHT];
                getcwd(currDir, MAX_LINE_LENGHT);
                char *args[MAX_CMD_SIZE];
                args[0] = cmd.tokens[0];

                args[1] = currDir;
                args[2] = NULL;

                exec_result = execvp(args[0], args);

                if (exec_result == -1) 
                {
                    perror("shell"); returnvalue = 0;
                }
            }
            else{
                char *args[MAX_CMD_SIZE];
                for(int i=0; i< cmd.n_arguments; i++){
                    args[i] = cmd.tokens[i];
                }
                args[cmd.n_arguments] = NULL;
                
                exec_result = execvp(args[0], args);

                if (exec_result == -1) 
                {
                    perror("shell"); returnvalue = 0;
                }
            }
		} 
		else // Parent process
		{

			do {
      			wpid = waitpid(pid, &status, WUNTRACED);
    		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
            
		}
		return returnvalue;
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
		char buffer[MAX_CMD_SIZE];
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