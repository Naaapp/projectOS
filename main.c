
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
        exit(0);
    }
}


int main(void)
{
    
    cmd = create_cmd();
    signal(SIGINT, sigint_h);
    result = 0;

    do {

        //ctrl + D
        if(feof(stdin)){
            free_cmd(cmd);
            exit(0);
        }

        if(result == -1){
            free_cmd(cmd);
            exit(0);
        }

        fflush(stdout);
        printf("%d$ ", result);
        fflush(stdout);
    

        read_line(cmd);

        result = launch(*cmd);

        fflush(stdout);
    } 
    while (1);
    
        
    free_cmd(cmd);

	return EXIT_SUCCESS;
}
