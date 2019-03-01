
/*******************************************************************************/

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "shell.h"

/*******************************************************************************/




int main(void)
{
  	int result;
    
    Cmd *cmd = create_cmd();

    do {
        printf("$ ");

        read_line(cmd);

        result = launch(*cmd);
    } 
    while (result);
    
    
        
    free_cmd(cmd);

	return EXIT_SUCCESS;
}
