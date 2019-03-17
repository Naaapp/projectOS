
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
	#include <fcntl.h>
	#include <termios.h>
	#include <signal.h>
	#include "shell.h"

	
	#define TOK_DEL " \t\r\n\a"

	#define MAX_LINE_LENGHT 256
	#define COMMAND_NOT_FOUND_ERROR 0

	static pid_t GBSH_PID;
	static pid_t GBSH_PGID;
	pid_t pid;

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
                    returnvalue = 0;
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
                    returnvalue = 0;
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
	        	int result =  (*builtin_func_no_arg[i]) ();
	            return result;
	        }
	    }
	    for (i = 0; i < lsh_num_builtins_arg(); i++) {
	        if (strcmp(cmd.tokens[0], builtin_str_arg[i]) == 0) {
	            return (*builtin_func_arg[i])(cmd);
	        }
	    }
	    return pipeHandler(cmd);
    }


    int pipeHandler(Cmd cmd){
    	
    	int filedes[2]; // pos. 0 output, pos. 1 input of the pipe
		int filedes2[2];
		int num_cmds = 1;
		char *command[256]; //contain the first command untreated

		pid_t pid, wpid;
		int status;

		int err = -1;
		int end = 0; //to stop the loop when no more argument

		int i = 0;
		int j = 0;
		int k = 0;
		int returnvalue = 0;



		//count the number of command
		while ( i < cmd.n_arguments && cmd.tokens[i] != NULL){
			if (strcmp(cmd.tokens[i],"|") == 0){
				num_cmds++;
			}
			i++;
		}

		//Loop, for each command
		while (j < cmd.n_arguments && (cmd.tokens[j] != NULL && end != 1)){



			k = 0;

			//Take the first command untreated of the cmd
			while (j < cmd.n_arguments && strcmp(cmd.tokens[j],"|") != 0){
				command[k] = cmd.tokens[j];
				j++;	
				if (j < cmd.n_arguments && cmd.tokens[j] == NULL){
					end = 1;
					k++;
					break;
				}
				k++;
			}

			printf("%s\n", command[0] );

			//
			command[k] = NULL;
			j++; 


			//Pipes for odd or event step
			if (i % 2 != 0){
				pipe(filedes); // for odd i
			}else{
				pipe(filedes2); // for even i
			}

			pid=fork();
		
			if(pid==-1){ //Error			
				if (i != num_cmds - 1){
					if (i % 2 != 0){
						close(filedes[1]); // for odd i
					}else{
						close(filedes2[1]); // for even i
					} 
				}			
				char* error = strerror(errno);
	        	printf("fork error: %s\n", error);
	        	return -1;
			}
			else if(pid==0){ //Child

				printf("child" );

				//first command
				if (i == 0){
					printf("first");
					dup2(filedes2[1], STDOUT_FILENO);
				}

				//last command
				else if (i == num_cmds - 1){
					printf("%s",last);
					if (num_cmds % 2 != 0){ // for odd number of commands
						dup2(filedes[0],STDIN_FILENO);
					}else{ // for even number of commands
						dup2(filedes2[0],STDIN_FILENO);
					}
				}

				//middle command
				else{ 
					printf("middle");
					// for odd i
					if (i % 2 != 0){
						dup2(filedes2[0],STDIN_FILENO); 
						dup2(filedes[1],STDOUT_FILENO);
					}
					// for even i
					else{ 
						dup2(filedes[0],STDIN_FILENO); 
						dup2(filedes2[1],STDOUT_FILENO);					
					} 
				}
				printf("exec");

				if (execvp(command[0],command)==err){
					kill(getpid(),SIGTERM);
				} 
				
			}
			 
			printf("common");
			//first command
			if (i == 0){
				printf("firstcommon");
				close(filedes2[1]);
			}

			//last command
			else if (i == num_cmds - 1){
				if (num_cmds % 2 != 0){ // for odd number of commands
					close(filedes[0]);
				}else{ // for even number of commands
					close(filedes2[0]);
				}
			}

			//middle command
			else{ 
				// for odd i
				if (i % 2 != 0){
					close(filedes2[0]);
					close(filedes[1]);
				}
				// for even i
				else{ 
					close(filedes[0]);
					close(filedes2[1]);				
				} 
			}


			if(pid == 1){
				printf("parent" );
				do {
	      			wpid = waitpid(pid, &status, WUNTRACED);
	    		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
    		}

			i++;

			return 1;
		}

		
	}


 