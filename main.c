
/*******************************************************************************/

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "shell.h"

/*******************************************************************************/

int result;
Cmd *cmd;

static void sigint_h(int sig)
{
    if(cmd != NULL)
        free_cmd(cmd);
    if(sig == SIGINT || sig == EOF){
        printf("\n");
        exit(0);
    }
}


int main(void)
{
    
    cmd = create_cmd();
    signal(SIGINT, sigint_h);

    do {
        printf("$ ");
        
        //ctrl + D
        if(feof(stdin)){
            printf("\n");
            free_cmd(cmd);
            exit(0);
        }

        read_line(cmd);

        result = launch(*cmd);
    } 
    while (result);
    
        
    free_cmd(cmd);

	return EXIT_SUCCESS;
}
