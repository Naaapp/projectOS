
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
     * free_tokens(Cmd *cmd);
     * free the memory used for the args of the command
     *
     * Parameters
     * cmd  a data structure of the command
    */

    static void free_tokens(Cmd *cmd);

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
        char **args = cmd.tokens;
	    if (args[1] == NULL || (strcmp(args[1],"~") == 0)) {
	        if (chdir(getenv("HOME")) != 0) {
	            perror("lsh");
	        }
	    }
	    else if ((strcmp(args[1],"..") == 0)){
	  	    if (chdir("../") != 0) {
	            perror("lsh");
	        }
	    }
	    else {
	        if (chdir(args[1]) != 0) {
	            perror("lsh");
	        }
	    }
	    return 1;
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
		int status;
		int returnvalue =1;

		pipe(fd);

		pid = fork();

		if (pid == -1)
		{
			char* error = strerror(errno);
        	printf("fork error: %s\n", error);
        	return;
		}
		else if (pid == 0) // Child process
		{
			int exec_result = 0;

			//Case 1 : lauch a programm in the current directory
			if(strlen(cmd.tokens[0]) > 2 && cmd.tokens[0][0] == '.' && cmd.tokens[0][1] == '/'){

				printf("case1\n");

                //Get the current directory
                char currDir[MAX_LINE_LENGHT];
                getcwd(currDir, MAX_LINE_LENGHT);

                strcat(currDir, "/");
                strcat(currDir, cmd.tokens[0]+2);
                cmd.tokens[0] = currDir;

                exec_result = execv(cmd.tokens[0], cmd.tokens);

                const char *msg = "finish";
				close(fd[0]);
				write(fd[1],msg,strlen(msg));

                exit(exec_result);
			}
			//Case 2 : absolute path 
			else if(strlen(cmd.tokens[0]) > 1 && cmd.tokens[0][0] ==  '/'){

				printf("case2\n");

	            exec_result = execv(cmd.tokens[0], cmd.tokens);

	            const char *msg = "finish";
				close(fd[0]);
				write(fd[1],msg,strlen(msg));

	            exit(exec_result);
            }
            //Case 3 : search in the PATH
            else {

            	printf("case3\n");

            	if (execvp(cmd.tokens[0], cmd.tokens) == -1) 
				{
			    perror("shell"); returnvalue = 0;
				}
				const char *msg = "finish";
				close(fd[0]);
				write(fd[1],msg,strlen(msg));

	   //          //Create a copy of the environment PATH to be sure to not modify it
	   //          char* env = getenv("PATH");
	   //          char PATH[strlen(env)];
	   //          strcpy(PATH, env);

	   //          //Cut PATH
	   //          size_t buffer_size = strlen(env);
	   //          char* path[buffer_size];
	   //          int i = 0;

	   //          path[i] = strtok(PATH, ":");
	   //          while(path[i++] != NULL)
	   //              path[i] = strtok(NULL,":");

	   //          //Memorize the command we want to execute
	   //          char exec_command[strlen(cmd.tokens[0])];
	   //          strcpy(exec_command, cmd.tokens[0]);

	   //          //Try to execute the command in every path folder
	   //          i = 0;
	   //          while(path[i]){

	   //              char buffer[MAX_LINE_LENGHT];

	   //              strcpy(buffer, path[i]);
	   //              strcat(strcat(buffer, "/") , exec_command);
	   //              cmd.tokens[0] = buffer;

	   //              exec_result = execv(cmd.tokens[0], cmd.tokens);
	   //          }

	   //          // Command wasn't found in PATH
	   //          printf("%s: command not found\n", exec_command);

	   //          const char *msg = "finish";
				// close(fd[0]);
				// write(fd[1],msg,strlen(msg));

	   //          exit(COMMAND_NOT_FOUND_ERROR);
        	}
		} 
		else // Parent process
		{
			char buf[64];
			close(fd[1]);
			memset(buf,0,sizeof(buf));
			read(fd[0],buf,sizeof(buf));
			if(strcmp(buf, "finish")){
				perror("shell");
			}
            
		}
		return returnvalue;
	}

    static void free_tokens(Cmd *cmd){
        if(cmd == NULL)
            return;
        if(cmd->tokens == NULL)
            return ;
        for(int i=0; i <cmd->n_arguments; i++){
            free(cmd->tokens[i]);
        }
        free(cmd->tokens);
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
        if(cmd->tokens != NULL)
            free_tokens(cmd);
        free(cmd);
    }

	int read_line(Cmd* cmd){
		int buffersize = SH_BUFFERSIZE;
		char *buffer = malloc(sizeof(char) * SH_BUFFERSIZE);
		char **token_array = malloc(sizeof(char*) * SH_TOKEN_BUFFERSIZE);
		char *token;
		int position = 0;
		int c;

		if (!buffer || !token_array) {
            fprintf(stderr, "shell: allocation error");
            exit(EXIT_FAILURE);
		}

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

			//In this case, reallocation of the buffer is needed
			if (!finish && position >= buffersize) 
			{
				buffersize += SH_BUFFERSIZE;
				buffer = realloc(buffer, buffersize);
				if (!buffer){
                    fprintf(stderr, "shell: allocation error\n");
                    exit(EXIT_FAILURE);
				}
			}
		}

		position = 0;
		buffersize = SH_TOKEN_BUFFERSIZE;

		//Loop to parse the line
		token = strtok(buffer, TOK_DEL);
        int n_args = 0;
		while (token != NULL) {
            char *token_copy = malloc(MAX_CMD_SIZE*sizeof(char));
            strcpy(token_copy, token);
			token_array[position] = token_copy;
	    	position++;

			//In this case, reallocation of the buffer is needed
			if (position >= buffersize){
				buffersize += SH_TOKEN_BUFFERSIZE;
				token_array = realloc(token_array, buffersize * sizeof(char*));
				if (!token_array){
                    fprintf(stderr, "shell: allocation error");
                    exit(EXIT_FAILURE);
				}
			}

			token = strtok(NULL, TOK_DEL);
            n_args++;
		}
        
		token_array[position] = NULL;
        if(cmd->tokens != NULL)
            free_tokens(cmd);
        
	  	cmd->tokens = token_array;
        cmd->n_arguments = n_args;
        
        free(buffer);
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