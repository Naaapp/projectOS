
/*
  Function Declarations for builtin shell commands:
 */

/*******************************************************************************/

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <linux/if_link.h>
#include <linux/if_packet.h>
#include "shell.h"
#include <netinet/in.h>


#define TOK_DEL " \t\r\n\a"

int specialreturnvalue = 1;

/*******************************************************************************/
static int lsh_num_builtins_arg();

static int lsh_num_builtins_no_arg();

/*
 * static int lsh_cd(Cmd cmd);
 * apply command cd
 *
 * Parameters
 * cmd  a data structure containing arguments
*/

static int lsh_cd(Cmd cmd);

/*
 * static int lsh_help;
 * apply command help
 *
*/

static int lsh_help(); 

/*
 * static int lsh_exit(Cmd cmd);
 * apply command exit
*/

static int lsh_exit();

/*
 * static int lsh_crypto;
 * apply command crypto
 *
 * Parameters
*/

static int lsh_sys(Cmd cmd);

/*
 * static int bash_launch_exec(Cmd cmd);
 * used to execute the given command
*/

static int bash_launch_exec(Cmd cmd);

struct interfaces_info
{
    char* name;
    char* address;
    unsigned int blocksize;
    char* netmask;
    char* macadress;
};

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str_no_arg[] = {
  "help",
  "exit",
};

char *builtin_str_arg[] = {
  "cd",
  "sys"
};

int (*builtin_func_no_arg[]) () = {
  &lsh_help,
  &lsh_exit,
};

int (*builtin_func_arg[]) (Cmd cmd) = {
  &lsh_cd,
  &lsh_sys
};

static int lsh_num_builtins_no_arg() {
  return sizeof(builtin_str_no_arg) / sizeof(char *);
}

static int lsh_num_builtins_arg() {
  return sizeof(builtin_str_arg) / sizeof(char *);
}

///////static function///////

static int lsh_sys (Cmd cmd) {
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    if(cmd.n_arguments == 1){
        return 0;
    }

    if(strcmp(cmd.tokens[1],"crypto")==0){
        fp = fopen("/proc/crypto", "r");
        if (fp == NULL)
            exit(EXIT_FAILURE);

        int blocksizefound = 1;
        while ((read = getline(&line, &len, fp)) != -1) {

            strtok(line, "\n"); //Remove the \n at the end of the line
            if(strncmp(line,"name",4)==0){ //If the line begin with "name"

                if(!blocksizefound) //If the blocksize of the previous name not printed because doesn't exist 
                    printf("blocksize : /\n");
                blocksizefound = 0;

                printf("%s | ", line);
            }
            if(strncmp(line,"blocksize",9)==0){ //If the line begin with "blocksize"
                printf("%s\n", line);
                blocksizefound = 1;
            }
        }

        fclose(fp);
        if (line)
            free(line);
    }
    else if(strcmp(cmd.tokens[1],"interfaces")==0){

        struct ifaddrs *ifaddr, *ifa;
        int family, s, n, s_;
        char host[NI_MAXHOST];
        char host_[NI_MAXHOST];

        if (getifaddrs(&ifaddr) == -1) {
           perror("getifaddrs");
           exit(EXIT_FAILURE);
        }

        /* Walk through linked list, maintaining head pointer so we
          can free list later */

        int size_int_info = 10;
        struct interfaces_info infos[size_int_info];
        size_t n_found=0;

           for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
                if (ifa->ifa_addr == NULL)
                    continue;

                family = ifa->ifa_addr->sa_family;

                /* Display interface name  */

                size_t found = 0;
                int interface_id = -1;
                for(size_t i=0;i<n_found;i++){
                    if(strcmp(infos[i].name, ifa->ifa_name)==0){
                        interface_id = i; //the id of the current interface 
                        found = 1;
                    }
                }
                if(!found){
                    interface_id = n_found;
                    n_found++;
                    infos[interface_id].name = ifa->ifa_name;
                }

                if (family == AF_INET ) {

                    /* For an AF_INET* interface address, display the address */
                    s = getnameinfo(ifa->ifa_addr,
                            sizeof(struct sockaddr_in),
                            host, NI_MAXHOST,
                            NULL, 0, NI_NUMERICHOST);
                    if (s != 0) {
                       printf("getnameinfo() failed: %s\n", gai_strerror(s));
                       exit(EXIT_FAILURE);
                    }

                    infos[interface_id].address = host;

                    /* For an AF_INET* interface address, display the netmask */
                    s_ = getnameinfo(ifa->ifa_netmask,
                            sizeof(struct sockaddr_in),
                            host_, NI_MAXHOST,
                            NULL, 0, NI_NUMERICHOST);
                    if (s_ != 0) {
                       printf("getnameinfo() failed: %s\n", gai_strerror(s_));
                       exit(EXIT_FAILURE);
                    }

                    infos[interface_id].netmask = host_;

                } else if (family == AF_PACKET && ifa->ifa_data != NULL) {

                    struct rtnl_link_stats *stats = ifa->ifa_data;

                    infos[interface_id].blocksize = stats->rx_packets;

                    char macp[INET6_ADDRSTRLEN];
                    struct sockaddr_ll *s = (struct sockaddr_ll*)(ifa->ifa_addr);
                    int i;
                    int len = 0;
                    for (i = 0; i < 6; i++) {
                        len += sprintf(macp+len, "%02X%s", s->sll_addr[i], i < 5 ? ":":"");
                    }
                    infos[interface_id].macadress = macp;
                }

            }

            int find = 0;
            if(cmd.n_arguments == 3){
                for (size_t i=0;i<n_found;i++){
                    if(strcmp(infos[i].name,cmd.tokens[2])==0){
                        printf("(%s) ip address:  %s | subnet mask: %s | mac address: %s\n", infos[i].name, infos[i].address, infos[i].netmask, infos[i].macadress );
                        find = 1;
                    }
                }
                if(!find)
                    printf("%s not found\n", cmd.tokens[2] );
            }
            if(cmd.n_arguments == 2){
                for (size_t i=0;i<n_found;i++){
                    printf("interface: %s | packet_received: %d\n", infos[i].name, infos[i].blocksize );
                }
            }

        freeifaddrs(ifaddr);
        }

        
    return 1;
}


static int lsh_cd(Cmd cmd){
    int returnvalue = 1;
    if (strcmp(cmd.tokens[1],"~") == 0 || cmd.n_arguments == 1){
        if (chdir(getenv("HOME")) != 0) {
            returnvalue = 0;
        }
    }
    else if ((strcmp(cmd.tokens[1],"..") == 0)){
        if (chdir("../") != 0) {
            returnvalue = 0;
        }
    }
    else {
        if (chdir(cmd.tokens[1]) != 0) {
            returnvalue = 0;
        }
    }
    return returnvalue;
}

static int lsh_help(){
    printf("Author : Theo Stassen, Ludovic Sangiovanni\n");
    fflush(stdout);
    return 1;
}

static int lsh_exit(){
    return -1;
}

static int bash_launch_exec(Cmd cmd)
{
    pid_t pid, tpid;
    int pfd[MAX_CMD_SIZE][2];

    char *args[MAX_CMD_SIZE];        
    int i = 0, pipe_locate[MAX_CMD_SIZE], pipe_count = 0;
    pipe_locate[0] = -1;

    for(i=0; i < cmd.n_arguments; i++){
        args[i] = cmd.tokens[i];
        if(strcmp(cmd.tokens[i], "|") == 0){
            pipe_count++;
            pipe_locate[pipe_count] = i;
            args[i] = NULL;
        }
    }
    args[i] = NULL;

    if(pipe_count == 0){
        int status;
        pid = fork();
        if (pid == -1)
            return -1;
     
        if (pid == 0){
            execvp(args[0], args);
            
            printf("\n");
            fflush(stdout);
            exit(0);
        }
        else{
            do{

                tpid = wait(&status);
                if(tpid != pid)
                    exit(tpid);                
            } while(tpid != pid);
            return WIFEXITED(status);
        }
    }

    for (i = 0; i < pipe_count + 1 && pipe_count != 0; i++) {
        if (i != pipe_count) pipe(pfd[i]);

        if (fork() == 0) {
            if (i == 0) {
                dup2(pfd[i][1], 1);
                close(pfd[i][0]); 
                close(pfd[i][1]);
            } else if (i == pipe_count) {
                dup2(pfd[i - 1][0], 0);
                close(pfd[i - 1][0]); 
                close(pfd[i - 1][1]);
            } else {
                dup2(pfd[i - 1][0], 0);
                dup2(pfd[i][1], 1);
                close(pfd[i - 1][0]); close(pfd[i - 1][1]);
                close(pfd[i][0]); close(pfd[i][1]);
            }

            execvp(args[pipe_locate[i] + 1], args + pipe_locate[i] + 1);
            exit(0);
        }
        else if (i > 0) {
            close(pfd[i - 1][0]); close(pfd[i - 1][1]);
        }
    }

    int status;

    for (i = 0; i < pipe_count + 1; i++) {
            wait(&status);
    }
    return WEXITSTATUS(status);
}

////end of static functions

Cmd *create_cmd(){
    Cmd *cmd = malloc(sizeof(Cmd));
    if(cmd == NULL)
        return NULL;
    cmd->n_arguments = 0;

    return cmd;           
}

void free_cmd(Cmd *cmd){
    if(cmd == NULL)
        return;
    free(cmd);
}

int read_line(Cmd* cmd){
    char buffer[MAX_CMD_SIZE*MAX_CMD_SIZE+1];
    char *token;
    int position = 0;
    int c;

    //Loop to read the line
    int finish = 0;
    while (!finish) {


        //We read the next character
        c = getchar();


        if (c == EOF || c == '\n') 
        //In this case we reached the end of the line, we have our buffer
        {
            buffer[position] = '\0';
            finish = 1;
        } 
        //In this case we put the character in the buffer
        else 
        {
            buffer[position] = c;
        }

        position++;
    }

    position = 0;

    //Loop to parse the line
    token = strtok(buffer, TOK_DEL);
    int n_args = 0;
    while (token != NULL) {
        strcpy(cmd->tokens[position], token);
        position++;
        token = strtok(NULL, TOK_DEL);
        n_args++;
    }

    cmd->n_arguments = n_args;

    return 0;
}


int launch(Cmd cmd){
    int i;

    if (cmd.tokens[0] == NULL) {
        // An empty command was entered.
        return 1;
    }
    for (i = 0; i < lsh_num_builtins_no_arg(); i++) {
        if (strcmp(cmd.tokens[0], builtin_str_no_arg[i]) == 0) {
            return (*builtin_func_no_arg[i]) ();
        }
    }
    for (i = 0; i < lsh_num_builtins_arg(); i++) {
        if (strcmp(cmd.tokens[0], builtin_str_arg[i]) == 0) {
            return (*builtin_func_arg[i])(cmd);
        }
    }
    return bash_launch_exec(cmd);
}