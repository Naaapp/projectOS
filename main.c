
/*******************************************************************************/

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "shell.c"
#include "shell.h"

/*******************************************************************************/




int main(void)
{
  	bash_loop();

	return EXIT_SUCCESS;
}
