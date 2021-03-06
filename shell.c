
/*
 * Shell implementation by Théo Stassen and Ludovic Sangiovanni
 * Project 2 Version
 * 27/03/19
 * shell.c
*/

/*******************************************************************************/

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
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


/*******************************************************************************/


/*
 * calculate number of builtin function
 *
 * Return
 * number of builtins functions
*/
static int lsh_num_builtins_arg();
static int lsh_num_builtins_no_arg();

/*
 * apply command cd
 *
 * Parameters
 * cmd  a data structure containing arguments of a command
 * Return 
 * value indicates the success/fail o the command
*/

static int lsh_cd(Cmd cmd);

/*
 * apply command help
 *
 * Parameters
 * cmd  a data structure containing arguments of a command
 * Return 
 * value indicates the success/fail of the command
*/

static int lsh_help(); 

/*
 * used to execute the given command
 *
 * Parameters
 * cmd  a data structure containing arguments of a command
 * Return 
 * value indicates the success/fail of the command
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
 *
 * Parameters
 * cmd  a data structure containing arguments of a command
 * Return 
 * value indicates the success/fail of the command
*/

static int bash_launch_exec(Cmd cmd);


/*
* structure used to collect information about the interfaces
*/

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

    // Case 1 : sys crypto
    if(strcmp(cmd.tokens[1],"crypto")==0){
        //Open the file which contain all the information needed
        fp = fopen("/proc/crypto", "r");
        if (fp == NULL)
            return 1;

        //Read the file line by line
        int blocksizefound = 1;
        while ((read = getline(&line, &len, fp)) != -1) {

            strtok(line, "\n"); //Remove the \n at the end of the line
            if(strncmp(line,"name",4)==0){ //If the line begin with "name"

                if(!blocksizefound){ //If the blocksize of the previous name not printed because doesn't exist 
                    printf("blocksize : /\n");
                    fflush(stdout);
                }
                blocksizefound = 0;

                printf("%s | ", line);
                fflush(stdout);
            }
            if(strncmp(line,"blocksize",9)==0){ //If the line begin with "blocksize"
                printf("%s\n", line);
                fflush(stdout);
                blocksizefound = 1;
            }
        }

        fclose(fp);
        if (line)
            free(line);
    }
    //Case 2 : sys interfaces or sys interfaces <name>
    else if(strcmp(cmd.tokens[1],"interfaces")==0){

        struct ifaddrs *ifaddr, *ifa;
        int family, s, n, s_;

        //We use the getifaddrs function to find the needed information 
        //in the linked list (ifaddr is the begin adress of it)
        if (getifaddrs(&ifaddr) == -1) {
           perror("getifaddrs");
           return 1;
        }

        // Walk through linked list, maintaining head pointer so we
        //  can free list later 
        int size_int_info = 50;
        struct interfaces_info infos4[size_int_info]; //Contain information of the interfaces (ipv4)
        struct interfaces_info infos6[size_int_info]; //Contain information of the interfaces (ipv6)
        size_t n_found=0; //Number of interfaces found
        
        //First walk to find the different interfaces and put the names in infos
        for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {            
            if (ifa->ifa_addr == NULL)
                continue;

            size_t found = 0;
            int interface_id = -1;

            //We verify if the current name already exists 
            for(size_t i=0;i<n_found;i++){
                if(strcmp(infos4[i].name, ifa->ifa_name)==0){
                    found = 1;
                }
            }
            
            //If it doesn't exit we add it to infos4
            if(!found){
                interface_id = n_found; //the id of the current interface 
                n_found++;
                infos4[interface_id].name = ifa->ifa_name;
            }
        }
        //Allocate memory for the infos
        for(size_t i=0; i < n_found; i++){
            infos4[i].address = malloc(sizeof(char) * NI_MAXHOST);
            strcpy(infos4[i].address, "");
            infos4[i].netmask = malloc(sizeof(char) * NI_MAXHOST);
            strcpy(infos4[i].netmask, "");
            infos4[i].macadress = malloc(sizeof(char) * INET6_ADDRSTRLEN);
            strcpy(infos4[i].macadress, "");
            infos6[i].address = malloc(sizeof(char) * NI_MAXHOST);
            strcpy(infos6[i].address, "");
            infos6[i].netmask = malloc(sizeof(char) * NI_MAXHOST);
            strcpy(infos6[i].netmask, "");
            infos6[i].macadress = malloc(sizeof(char) * INET6_ADDRSTRLEN);
            strcpy(infos6[i].macadress, "");
        }

        //Second walk to put the information needed in the infos structure
        for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
            int interface_id;
            
            if (ifa->ifa_addr == NULL)
                continue;
            
            //Family of information we can find in this current ifa
            family = ifa->ifa_addr->sa_family;

            //We find the name of the current interface we can find some information
            for(size_t i=0; i < n_found; i++){
                if(strcmp(infos4[i].name, ifa->ifa_name)==0)
                    interface_id = i;
            }

            //Case 1 : we can find the ip address and the netmask
            if (family == AF_INET) {

                //We extract the adress and put it into info4
                s = getnameinfo(ifa->ifa_addr,
                        sizeof(struct sockaddr_in),
                        infos4[interface_id].address, NI_MAXHOST,
                        NULL, 0, NI_NUMERICHOST);
                if (s != 0) {
                    printf("getnameinfo() failed: %s\n", gai_strerror(s));
                    fflush(stdout);
                    return 1;
                }

                //We extract the netmask and put it into info4
                s_ = getnameinfo(ifa->ifa_netmask,
                        sizeof(struct sockaddr_in),
                        infos4[interface_id].netmask, NI_MAXHOST,
                        NULL, 0, NI_NUMERICHOST);
                if (s_ != 0) {
                    printf("getnameinfo() failed: %s\n", gai_strerror(s_));
                    fflush(stdout);
                    return 1;
                }

            } 
            //Case 2 : we can find the mac adress and the number of packets received
            else if (family == AF_PACKET && ifa->ifa_data != NULL) {

                //Structure which contain the information about packets
                struct rtnl_link_stats *stats = ifa->ifa_data;

                //We find the number of packet received
                infos4[interface_id].blocksize = stats->rx_packets;

                //We find the MAC adress and put it into macp
                char macp[INET6_ADDRSTRLEN];
                struct sockaddr_ll *s = (struct sockaddr_ll*)(ifa->ifa_addr);
                int i;
                int len = 0;
                for (i = 0; i < 6; i++) {
                    len += sprintf(macp+len, "%02X%s", s->sll_addr[i], i < 5 ? ":":"");
                }
                strcpy(infos4[interface_id].macadress, macp);
            }
            //Optional Case :  we can find the ipv6 adress and subnet mask 
            else if(family == AF_INET6){

                //We extract the adress and put it into info4
                s = getnameinfo(ifa->ifa_addr,
                        sizeof(struct sockaddr_in6),
                        infos6[interface_id].address, NI_MAXHOST,
                        NULL, 0, NI_NUMERICHOST);
                if (s != 0) {
                    printf("getnameinfo() failed: %s\n", gai_strerror(s));
                    fflush(stdout);
                    return 1;
                }
                
                //We extract the netmask and put it into info4
                s_ = getnameinfo(ifa->ifa_netmask,
                        sizeof(struct sockaddr_in6),
                        infos6[interface_id].netmask, NI_MAXHOST,
                        NULL, 0, NI_NUMERICHOST);
                if (s_ != 0) {
                    printf("getnameinfo() failed: %s\n", gai_strerror(s_));
                    fflush(stdout);
                    return 1;
                }
              
            }
        }

        //If the command was interface <name> we search the corresponding name in infos and print the information
        int find = 0;
        if(cmd.n_arguments == 3){
            for (size_t i=0;i<n_found;i++){
                if(strcmp(infos4[i].name, cmd.tokens[2])==0){
                    printf("(%s)", infos4[i].name);
                    printf(" ipv4 address: %s", infos4[i].address);
                    printf(" | subnet mask: %s", infos4[i].netmask);
                    printf(" | mac adress: %s", infos4[i].macadress);
                    printf("\n");

                    //This optional part print the information about ipv6 interfaces

                    /*
                    if(strcmp(infos6[i].address, "") || strcmp(infos6[i].netmask, "") || strcmp(infos6[i].macadress, "")){
                        printf("(%s)", infos4[i].name);
                        printf(" ipv6 address: %s", infos6[i].address);
                        printf(" | subnet mask: %s", infos6[i].netmask);
                        printf(" | mac adress: %s", infos6[i].macadress);
                        printf("\n");
                    }
                    */

                    fflush(stdout);
                    find = 1;
                }
            }
            if(!find){
                printf("%s not found\n", cmd.tokens[2] );
                fflush(stdout);
            }
        }

        //If the command was interfaces we needed info of all the interfaces stored
        if(cmd.n_arguments == 2){
            for (size_t i=0;i<n_found;i++){
                printf("interface: %s | packet_received: %d\n", infos4[i].name, infos4[i].blocksize );
                fflush(stdout);
            }
        }
        
        for(size_t i=0; i < n_found; i++){
            free(infos4[i].address);
            free(infos4[i].netmask);
            free(infos4[i].macadress);
        }

        freeifaddrs(ifaddr);
    }
    //Case 3: infos
    else if(strcmp(cmd.tokens[1],"infos")==0){
        struct stat sb;

        //If no arguments of infos we use the current directory
        char* path;
        if(cmd.n_arguments == 2){
            char cwd[256];
            getcwd(cwd, sizeof(cwd));
            path = cwd;
        }
        //If arguments of inofs we use it as directory
        else if(cmd.n_arguments == 3){
            path = cmd.tokens[2];
        }

        //The funtion lstat gies in sb the infos about the directory / file  of the given path
        if (lstat(path, &sb) == -1) {
            printf("cannot find the path\n");
            fflush(stdout);
            return 1;
        }

        //We print the information needes
        switch (sb.st_mode & S_IFMT) {
            case S_IFBLK:  printf("type: S_IFBLK\n");            break;
            case S_IFCHR:  printf("type: S_IFCHR\n");            break;
            case S_IFDIR:  printf("type: S_IFDIR\n");            break;
            case S_IFIFO:  printf("type: S_IFIFO\n");            break;
            case S_IFLNK:  printf("type: S_IFLNK\n");            break;
            case S_IFREG:  printf("type: S_IFREG\n");            break;
            case S_IFSOCK: printf("type: S_IFSOCK\n");           break;
            default:       printf("type: unknown\n");            break;
        }
        fflush(stdout);

        printf("inode number: %ld\n", (long) sb.st_ino);
        printf("total size: %lld bytes\n", (long long) sb.st_size);
        printf("number of blocks: %lld blocks\n", (long long) sb.st_blocks);
        printf("last file modification: %s", ctime(&sb.st_mtime));
        fflush(stdout);
    }
    else
        return 1;

    return 0;
}


static int lsh_cd(Cmd cmd){
    int returnvalue = 0;
    //Case 1 : cd ~ 
    if (strcmp(cmd.tokens[1],"~") == 0 || cmd.n_arguments == 1){
        if (chdir(getenv("HOME")) != 0) {
            returnvalue = 1;
        }
    }
    //Case 2 : cd ..
    else if ((strcmp(cmd.tokens[1],"..") == 0)){
        if (chdir("../") != 0) {
            returnvalue = 1;
        }
    }
    //Case 3 : cd dirname, cd /dirname, cd ./dirname
    else {
        if (chdir(cmd.tokens[1]) != 0) {
            returnvalue = 1;
        }
    }
    return returnvalue;
}

static int lsh_help(){
    printf("Author : Theo Stassen, Ludovic Sangiovanni\n");
    fflush(stdout);
    return 0;
}

static int lsh_exit(){
    return -1;
}

static int bash_launch_exec(Cmd cmd)
{
    //Contain the tokens but with NULL instead "|" and a NULL at the end 
    //to be usable by the pipe execution. 
    char *args[MAX_CMD_SIZE];

    //Pipe declarations
    pid_t pid, tpid;
    int pfd[MAX_CMD_SIZE][2];
    int i = 0, pipe_locate[MAX_CMD_SIZE], pipe_count = 0;
    pipe_locate[0] = -1;

    //Search "|" to know if pipes needed
    for(i=0; i < cmd.n_arguments; i++){
        args[i] = cmd.tokens[i];
        if(strcmp(cmd.tokens[i], "|") == 0){
            pipe_count++;
            pipe_locate[pipe_count] = i;
            args[i] = NULL;
        }
    }
    args[i] = NULL;

    //No pipe needed, normal execution (single fork)
    if(pipe_count == 0){
        int status;

        pid = fork();
        //Error when forking
        if (pid == -1)
            return -1;
        //Child process
        if (pid == 0){
            execvp(args[0], args);
            
            printf("\n");
            fflush(stdout);
            exit(-1);
        }
        //Parent process
        else{
            do{
                tpid = wait(&status);
                if(tpid != pid)
                    exit(tpid);                
            } while(tpid != pid);
            return WEXITSTATUS(status);
        }
    }

    //Pipes needed, loop which manage the different commands and execute them
    for (i = 0; i < pipe_count + 1 && pipe_count != 0; i++) {
        
        if (i != pipe_count) pipe(pfd[i]);

        if (fork() == 0) {
            fflush(stdout);
            if (i == 0) {
                dup2(pfd[i][1], 1);
                close(pfd[i][0]); 
                close(pfd[i][1]);
            } 
            //Last command
            else if (i == pipe_count) {
                dup2(pfd[i - 1][0], 0);
                close(pfd[i - 1][0]); 
                close(pfd[i - 1][1]);
            } 
            //Another command
            else {
                dup2(pfd[i - 1][0], 0);
                dup2(pfd[i][1], 1);
                close(pfd[i - 1][0]); close(pfd[i - 1][1]);
                close(pfd[i][0]); close(pfd[i][1]);
            }

            //Execute current command
            execvp(args[pipe_locate[i] + 1], args + pipe_locate[i] + 1);
            
            exit(1);
        }
        //Parent process
        else if (i > 0) {
            close(pfd[i - 1][0]); close(pfd[i - 1][1]);
        }
    }
    int status, stat_ret;

    //Wait the end of all commands
    for (i = 0; i < pipe_count + 1; i++) {
        wait(&status);
        if(WEXITSTATUS(status))
            stat_ret = status;
    }
    return WEXITSTATUS(stat_ret);
}

////end of static functions//////////

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

        //In this case we reached the end of the line, we have our buffer
        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            finish = 1;
        } 
        //In this case we put the character in the buffer
        else {
            buffer[position] = c;
        }
        position++;
    }
    position = 0;

    //Loop to parse the line
    token = strtok(buffer, TOK_DEL);
    int n_args = 0;
    while (token != NULL) {
        if(!strcmp(token, "|")){
            strcpy(cmd->tokens[position], token);
            position++;
        }
        else if(token[0] == '|'){
            strcpy(cmd->tokens[position], "|");
            position++;
            n_args++;
            token++;
            strcpy(cmd->tokens[position], token);
            position++;
        }
        else{
            strcpy(cmd->tokens[position], token);
            position++;
        }
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

    //We check if the entered command is one of the builtin (whitout arguments)
    for (i = 0; i < lsh_num_builtins_no_arg(); i++) {
        if (strcmp(cmd.tokens[0], builtin_str_no_arg[i]) == 0) {
            return (*builtin_func_no_arg[i]) ();
        }
    }
    //We check if the entered command is one of the builtin (with arguments)
    for (i = 0; i < lsh_num_builtins_arg(); i++) {
        if (strcmp(cmd.tokens[0], builtin_str_arg[i]) == 0) {
            return (*builtin_func_arg[i])(cmd);
        }
    }
    //If no one of these case, we try to execute the command
    return bash_launch_exec(cmd);
}