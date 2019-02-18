
//  shell.h
//  Shell
//

#ifndef shell_h
#define shell_h

#define MAX_CMD_SIZE 256

/* Opaque Structure */
typedef struct strug_arg Cmd;

int read_line(Cmd* cmd);
int launch(Cmd cmd);

#endif /* shell_h */
