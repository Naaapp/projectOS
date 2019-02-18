
//  shell.h
//  Shell
//

#ifndef shell_h
#define shell_h

#define MAX_CMD_SIZE 256

/* Opaque Structure */
typedef struct Cmds {
	int n_arguments;
	char** tokens;
} Cmd;

/*int bash_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);*/
void bash_loop(void);
int read_line(Cmd* cmd);
int launch(Cmd cmd);

#endif /* shell_h */
