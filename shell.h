
//  shell.h
//  Shell
//

#ifndef shell_h
#define shell_h

#define MAX_CMD_SIZE 256

/* Opaque Structure */
typedef struct cmd_t Cmd;

struct cmd_t{
	int n_arguments;
	char tokens[MAX_CMD_SIZE][MAX_CMD_SIZE];
};


Cmd *create_cmd();
void free_cmd(Cmd *cmd);
int read_line(Cmd* cmd);
int launch(Cmd cmd);




#endif /* shell_h */
