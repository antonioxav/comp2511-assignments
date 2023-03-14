/*
    COMP3511 Spring 2023 
    PA1: Simplified Linux Shell (myshell)

    Your name: Srijan Saxena
    Your ITSC email: ssaxenaad@connect.ust.hk 

    Declaration:

    I declare that I am not involved in plagiarism
    I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks. 

*/

// Note: Necessary header files are included
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/stat.h> // For constants that are required in open/read/write/close syscalls
#include <sys/wait.h> // For wait() - suppress warning messages
#include <fcntl.h> // For open/read/write/close syscalls

// Assume that each command line has at most 256 characters (including NULL)
#define MAX_CMDLINE_LEN 256

// Assume that we have at most 8 arguments
#define MAX_ARGUMENTS 8

// Assume that we only need to support 2 types of space characters: 
// " " (space) and "\t" (tab)
#define SPACE_CHARS " \t"

// The pipe character
#define PIPE_CHAR "|"

// Assume that we only have at most 8 pipe segements, 
// and each segment has at most 256 characters
#define MAX_PIPE_SEGMENTS 8

// Assume that we have at most 8 arguments for each segment
// We also need to add an extra NULL item to be used in execvp
// Thus: 8 + 1 = 9
//
// Example: 
//   echo a1 a2 a3 a4 a5 a6 a7 
//
// execvp system call needs to store an extra NULL to represent the end of the parameter list
//
//   char *arguments[MAX_ARGUMENTS_PER_SEGMENT]; 
//
//   strings stored in the array: echo a1 a2 a3 a4 a5 a6 a7 NULL
//
#define MAX_ARGUMENTS_PER_SEGMENT 9

// Define the  Standard file descriptors here
#define STDIN_FILENO    0       // Standard input
#define STDOUT_FILENO   1       // Standard output 

// Define some templates for printf
#define TEMPLATE_MYSHELL_START "Myshell (pid=%d) starts\n"
#define TEMPLATE_MYSHELL_END "Myshell (pid=%d) ends\n"


 
// This function will be invoked by main()
// TODO: Implement the multi-level pipes below
void process_cmd(char *cmdline);

// This function will be invoked by main()
// TODO: Replace the shell prompt with your own ITSC account name
void show_prompt();


// This function will be invoked by main()
// This function is given. You don't need to implement it.
int get_cmd_line(char *cmdline);

// This function helps you parse the command line
// read_tokens function is given. You don't need to implement it.
//
// Suppose the following variables are defined:
//
// char *pipe_segments[MAX_PIPE_SEGMENTS]; // character array buffer to store the pipe segements
// int num_pipe_segments; // an output integer to store the number of pipe segment parsed by this function
// char cmdline[MAX_CMDLINE_LEN]; // The input command line
//
// Sample usage:
//
//  read_tokens(pipe_segments, cmdline, &num_pipe_segments, "|");
// 
void read_tokens(char **argv, char *line, int *numTokens, char *token);

void print_arr(char **arr, int n);


/* The main function implementation */
int main()
{
    char cmdline[MAX_CMDLINE_LEN];
    printf(TEMPLATE_MYSHELL_START, getpid());

    // The main event loop
    while (1)
    {
        show_prompt();
        if (get_cmd_line(cmdline) == -1)
            continue; /* empty line handling */

        // Implement the exit command 
        if ( strcmp(cmdline, "exit") == 0 ) {
            printf(TEMPLATE_MYSHELL_END, getpid());
            exit(0);
        }

        pid_t pid = fork();
        if (pid == 0) {
            // the child process handles the command
            process_cmd(cmdline);
            // the child process terminates without re-entering the loop
            exit(0); 
        } else {
            // the parent process simply waits for the child and do nothing
            wait(0);
            // the parent process re-enters the loop and handles the next command
        }
            
    }
    return 0;
}

void process_cmd(char *cmdline)
{
    // Un-comment this line if you want to know what is cmdline input parameter
    printf("The input cmdline is: %s\n", cmdline);
    int status;

    // * Get Pipe segments
    char *pipe_segments[MAX_PIPE_SEGMENTS];
    int num_pipe_segments;
    read_tokens(pipe_segments, cmdline, &num_pipe_segments, PIPE_CHAR);
    print_arr(pipe_segments, num_pipe_segments);
    
    int prev_pfds[2]; // previous pipe
    int next_pfds[2]; // next pipe
    // * Start pipe loop
    for (int i=0;i<num_pipe_segments;i++){
        /*
            ? Fork a child for each pipe segment
            ? Create a pipe for each segment. Pipe conects child to child. 
            ? Pipe created by parent.
            ? Pipe is created only if i!=(num_pipe_segments-1)
            ? Within child, i!=0, stdin is replaced with pfds[0]
            ? Within child, i!=(num_pipe_segments-1), stdout is replaced with pfds[1]
            ? Tokenize command and execute
        */

        // * Create next pipe if segment is not last pipe segment
        if (i!=(num_pipe_segments-1)) pipe(next_pfds);

        // * Create child to execute segment
        pid_t child_pid = fork();

        if (child_pid==-1){
            printf("child no create");
        }
        else if (child_pid==0){
            // * replace stdin with prev_pfds[0] for multiple pipes
            if (i!=0) {
                close(0); // Close stdin
                dup2(prev_pfds[0],0); // set read of prev_pipe as stdin
                close(prev_pfds[1]); // don't need write of prev_pipe
            }
            
            // * replace stdout with next_pfds[1] for all except last pipe
            if (i!=(num_pipe_segments-1)){
                close(1); // Close stdout
                dup2(next_pfds[1],1); // set write of next_pipe as stdout
                close(next_pfds[0]); // don't need read of next_pipe
            }

            // * Get arguments
            char *cmd_args[MAX_ARGUMENTS];
            int num_args;
            read_tokens(cmd_args, pipe_segments[i], &num_args, SPACE_CHARS);
            // print_arr(cmd_args, num_args);

            // * add NULL for exp args
            char *arguments[MAX_ARGUMENTS_PER_SEGMENT];
            for (int a=0; a<num_args; a++){
                arguments[a] = cmd_args[a];
            }
            arguments[num_args] = NULL;
            // print_arr(arguments, num_args+1);

            execvp(arguments[0],arguments);
            // printf("%s failed",arguments[0]);
        }
        else {
            wait(&status);
            printf("%i\n",status);
        }
    }


    // TODO: Write your program to handle the command
}

// Implementation of read_tokens function
void read_tokens(char **argv, char *line, int *numTokens, char *delimiter)
{
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}

// Implementation of show_prompt
void show_prompt()
{
    // TODO: replace the shell prompt with your ITSC account name
    // For example, if you ITSC account is cspeter@connect.ust.hk
    // You should replace ITSC with cspeter
    printf("ssaxenaad> ");
}

// Implementation of get_cmd_line
int get_cmd_line(char *cmdline)
{
    int i;
    int n;
    if (!fgets(cmdline, MAX_CMDLINE_LEN, stdin))
        return -1;
    // Ignore the newline character
    n = strlen(cmdline);
    cmdline[--n] = '\0';
    i = 0;
    while (i < n && cmdline[i] == ' ') {
        ++i;
    }
    if (i == n) {
        // Empty command
        return -1;
    }
    return 0;
}

void print_arr(char **arr, int n){
    for(int i = 0; i < n; i++){
        printf(arr[i]);
        printf("`\n");
    }
}