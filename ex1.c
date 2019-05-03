/*
CREATED BY: Arbel Hizmi 
Program description:
The following program is building a basic shell.
the program will get a command from the user by using fgets and insert the command into buffer. 
By using get command function the program split the command that came from the user
and insert the command to the first place, after arguments and finally null.
The program would create a process by using fork, and in the process send the array of the command to execvp .
I did main function, that all the rest of the function happens in it - commandLoop.
in commandLoop all the logic of the program is happening, calling to the different functions
*/


#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>


#define SIZE 510
#define SIZE_OF_ARGS 2
#define INDEX_FOR_DELETE " \n"
#define ARGS_SIZE 2
#define PIPE_CHARS "|\n"
#define FILE_IN 4
#define FILE_OUT 1
#define FILE_OUT_APPEND 2
#define FILE_ERR 3


int total_length_of_commands = 0;
int num_of_commands = 0;
int pipe_flag = 0;

void commandLoop();
void getPath();
//char* readLine(char* buff);
char** splitCommand(char* command);
void exec_cmd(char **args);
void free_arguments(char **args, char *cmd_copy);
void search_in_command(char* cmd);
char** split_pipe(char* cmd);
void exec_pipe(char **left_cmd, char **right_cmd);
int check_redirection(char *buffer);
char *get_file_name(char* command);

int main() 
{
    commandLoop();
    
    return 0;
}

//all the functions of the program happens inside this function.
//inside while loop, there are calls for the diffrent functions of the program
void commandLoop() {
    char *command;
    char **args;
    char* cmd_copy;
    char buff[SIZE];
    char *file_name;
    int numOfComs;
    double average;


    while(1){
        getPath();
        fgets(buff, SIZE, stdin);
        command = buff;
        total_length_of_commands += strlen(buff);
        if (command[0] == '\n') {
            continue;
        }
        search_in_command(command);

        cmd_copy = (char*)malloc(sizeof(char) * (strlen(command) + 1));
        strncpy(cmd_copy, command, (strlen(command) + 1));
        num_of_commands++;
        

        if (pipe_flag == 1) {       //pipe flag is on and there is '|' in the command
            
            char **commads_arr = split_pipe(cmd_copy);
            char *left = commads_arr[0];
            char *right = commads_arr[1];
            char **left_cmd = splitCommand(left);
            char **right_cmd = splitCommand(right);
            exec_pipe(left_cmd, right_cmd);
            pipe_flag = 0;
            free(commads_arr);
            continue;
        }

        args = splitCommand(cmd_copy);
        if (strcmp(command, "done\n") == 0) {
            average = (double)total_length_of_commands / (double)num_of_commands;
            printf("Num of commands: %d\n", num_of_commands);
            printf("Total length of all commands: %d\n", total_length_of_commands);
            printf(" Average length of all commands: %lf\n", average);
            printf(" See you Next time !\n");
            free_arguments(args, cmd_copy);
            break; 
        } else if (strcmp(command, "cd\n") == 0) {
            printf("Comand not supperted (Yet)\n");
            free_arguments(args, cmd_copy);
            continue;
        } else {
            
            exec_cmd(args);
        }
        free_arguments(args, cmd_copy);
    }
    
    
}

//a function that get the path, and print it to the screen in the requested format
void getPath() {
    struct passwd *pws;
    long size;
    char *buf;
    char *ptr;
    uid_t uid = 0; //changs - added this line
    size = pathconf(".", _PC_PATH_MAX);
        if ((buf = (char*)malloc((size_t)size)) != NULL){
            ptr = getcwd(buf, (size_t)size);
        }
        if ((pws = getpwuid(uid)) == NULL) {
            perror("getpwuid() error");
        } else {
            //pws = getpwuid(geteuid());
            printf("%s@%s>", pws->pw_name, ptr);
            free(buf);
        }
}


//the function split the command the user to the different words.
//the function use strtok to split the command 
char** splitCommand(char* command) {
    int args_size = SIZE_OF_ARGS, index = 0;
    char **words = (char**)malloc(args_size * sizeof(char*));
    assert(words != NULL);
    char *word;

    int check_redirect = check_redirection(command);
    char *operand;
    switch (check_redirect)
    {
        case 1:
            operand = strtok(command, '>');
            
            break;
        case 2:
            operand = strtok(command, '>'); //the operator >>
            break;
        case 3:
            operand = strtok(command, '2'); //the operator 2>
            break;
        case 4:
            operand = strtok(command, '<');
            break;
        default:
            break;
    }

    word = strtok(command, INDEX_FOR_DELETE);

    while(word != NULL) {
        words[index] = word;
        index++;
        args_size++;
        words = (char**)realloc(words, args_size * sizeof(char*));
        assert(words != NULL);
        
        word = strtok(NULL, INDEX_FOR_DELETE);
    }
    words[index] = NULL;
    return words;
}

//the exec funcion create another processv - child process - that would occure
//while the "father process" is happening
void exec_cmd(char **args) {
    pid_t pid;
    pid = fork();
    if (pid == 0) {
        //child process
        if (execvp(args[0], args) == -1) {
            perror("exec failed");
            exit(EXIT_FAILURE);
        }
        
    } else if (pid < 0) {
        //Error forking
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else {
        //parent process
        wait(NULL);
    }
}

//function that callad to free the arguments which located on the heap
void free_arguments(char **args, char *cmd_copy) {
    free(cmd_copy);
    free(args);
}

void search_in_command(char* cmd) {
    int i;
    for (i = 0; i < strlen(cmd) + 1; i++) {
        if (cmd[i] == '|') {
            pipe_flag = 1;
        }
    }
}

char** split_pipe(char* cmd) {
    int position = 0;
    char **command = (char**)malloc(sizeof(char*) * ARGS_SIZE);
    assert(command != NULL);
    char *temp;
    temp = strtok(cmd, PIPE_CHARS);
    while (temp != NULL) {
        command[position] = temp;
        position++;
        temp = strtok(NULL, PIPE_CHARS);
    }

    return command;
}


void exec_pipe(char **left_cmd, char **right_cmd) {
    pid_t left_child;
    pid_t right_child;
    int check_dup;
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("pipe failure");
        exit(EXIT_FAILURE);
    }
    
    left_child = fork();
    if (left_child < 0) {
        //Error forking
        perror("fork_left failed");
        exit(EXIT_FAILURE);
    }
    if (left_child == 0) {
        
        check_dup = dup2(pipe_fd[1], STDOUT_FILENO); //writing to the pipe
        if (check_dup == -1) {
            perror("dup failure");
            exit(EXIT_FAILURE);
        }
        close(pipe_fd[0]);      //closing the reading side
        close(pipe_fd[1]);      //closing the write side after writing

        if (execvp(left_cmd[0], left_cmd) == -1) {
            perror("exec_left failed");
            exit(EXIT_FAILURE);
        } 
    } 

    right_child = fork();
    if (right_child < 0) {
        //Error forking
        perror("fork_left failed");
        exit(EXIT_FAILURE);
    }
    if (right_child == 0) {
       
        check_dup = dup2(pipe_fd[0], STDIN_FILENO); //reading from the pipe
        if (check_dup == -1) {
            perror("dup failure");
            exit(EXIT_FAILURE);
        }
        close(pipe_fd[1]);      //closing the writing side
        close(pipe_fd[0]);      //closing the reading side after read

        if (execvp(right_cmd[0], right_cmd) == -1) {
            perror("exec_left failed");
            exit(EXIT_FAILURE);
        } 
    } 
    close(pipe_fd[1]);
    close(pipe_fd[0]);
    //parent process
    wait(NULL);
    wait(NULL);
}

int check_redirection(char *buffer){
    char *ptr ;
    if ((ptr = strchr(buffer,'>')) != NULL) {
        if (*(++ptr) == '>'){
            return FILE_OUT_APPEND; 
        }
        --ptr;
        if(*(--ptr) == 2){
        return FILE_ERR;
        }
        return FILE_OUT;
    }

    if ((ptr = strchr(buffer,'<')) != NULL) {
    return FILE_IN;
    }
    // if no redirection return 0;
    return 0;
}

char *get_file_name(char* command) {

}