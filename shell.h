
/*
 * Shell implementation by Théo Stassen and Ludovic Sangiovanni
 * 20/03/19
 * shell.h
*/

#ifndef shell_h
#define shell_h

#define MAX_CMD_SIZE 256

/* Opaque Structure */
typedef struct cmd_t Cmd;

struct cmd_t{
	int n_arguments;
	char tokens[MAX_CMD_SIZE][MAX_CMD_SIZE];
};

/*
 * free data structure containing arguments
 *
 * Parameters
 * cmd  a data structure containing arguments
*/
Cmd *create_cmd();

/*
 * free data structure containing arguments
 *
 * Parameters
 * cmd  a data structure containing arguments
*/
void free_cmd(Cmd *cmd);

/*
 * read a line and parse it into tokens, put in cmd
 *
 * Parameters
 * cmd  a pointer to data structure containing arguments
 * Return 
 * 0 (success) or 1(fail)
*/
int read_line(Cmd* cmd);

/*
 * lauch a command
 *
 * Parameters
 * cmd  a data structure containing arguments of a command
 * Return 
 * 0 (success) or 1(fail)
*/
int launch(Cmd cmd);




#endif /* shell_h */
