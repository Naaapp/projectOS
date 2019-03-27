
/*
 * Shell implementation by Théo Stassen and Ludovic Sangiovanni
 * 20/03/19
 * main.c
*/

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

/*******************************************************************************/

static void sigint_h(int sig)
{
    //Free the cmd and success exit 
    if(cmd != NULL)
        free_cmd(cmd);
    if(sig == SIGINT || sig == EOF){
        exit(EXIT_SUCCESS);
    }
}


int main(void)
{
    //Allocate a new cmd structure
    cmd = create_cmd();
    //Lauch a signal used to free cmd at the end
    signal(SIGINT, sigint_h);
    result = 0;

    //At each iteration, print ret$ , 
    //read the line writed by the user, parse it and lauch it
    do {
        printf("%d$ ", result);
        fflush(stdout);
    
        //If the ctrl + D is pressed
        if(feof(stdin)){
            free_cmd(cmd);
            exit(EXIT_SUCCESS);
        }

        //Read next line and parse it
        read_line(cmd);



        //Lauch the line, result is -1, 0 or 1
        result = launch(*cmd);
        if(result == -1){
            free_cmd(cmd);
            exit(EXIT_SUCCESS);
        }
    } 
    while (1);

    return EXIT_SUCCESS;
}