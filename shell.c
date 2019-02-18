
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
#include "shell.h"

/*******************************************************************************/


int read_line(Cmd* cmd)
{
	return 1;
}

int launch(Cmd cmd)
{
	return 1;
}

void bash_loop(void)
{
	Cmd cmd;
	int result;

	do {
    printf("> ");
    if(!read_line(cmd*)){
    	fprintf(stderr, "bash: read_line error\n");
    }

    result = lauch(cmd);

    free(cmd);
	} 
	while (result);

}