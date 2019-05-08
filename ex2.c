/*
CREATED BY: Arbel Hizmi
Program Description:
The following program is building a basic shell.
the program will get a command from the user by using fgets and insert the command into buffer. 
By using splitCommand function (preparing for the exec function)the program split the command that came from the user
and insert the command to the first place, after arguments and finally null.
The program would create a process by using fork, and in the process send the array of the command to execvp.
In commandLoop all the logic of the program is happening, calling to the different functions
the program would also support redirection and pipe, with using pipe and dup2 functions
the program would support 4 types of commands: 
1. pipe and redirection 2. pipe only 3. redirection only 4. no pipe and no redirection
the program would use fork, pipe and dup2 functions for the different process
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
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
#define FILE_IN "<\n"          //number 4
#define FILE_OUT ">\n"         //number 1
#define FILE_OUT_APPEND ">>\n" //number 2
#define FILE_ERR "2>\n"        //number 3

int total_length_of_commands = 0;
int num_of_commands = 0;
int pipe_flag = 0;

void commandLoop();
void getPath();
char **splitCommand(char *command);
void exec_cmd(char **args, char *file_name, int sign);
void free_arguments(char **args, char *cmd_copy);
void search_pipe_in_command(char *cmd);
char **split_pipe(char *cmd, int sign);
void exec_pipe(char **left_cmd, char **right_cmd, char *file_name, int sign);
int check_redirection(char *buffer);
void redirection(char *file_name, int sign);

int main()
{
    commandLoop();

    return 0;
}

//all the functions of the program happens inside this function.
//inside while loop, there are calls for the diffrent functions of the program
void commandLoop()
{
    char *command;
    char **args;
    char *cmd_copy;
    char buff[SIZE];
    char *sub_right_right;
    int numOfComs;
    double average;

    while (1)
    {
        getPath();
        fgets(buff, SIZE, stdin);
        command = buff;
        total_length_of_commands += strlen(buff);
        if (command[0] == '\n')
        {
            continue;
        }
        search_pipe_in_command(command);                    //to search for the | char in the command
        int redirection_check = check_redirection(command); //checking for redirection in the command

        cmd_copy = (char *)malloc(sizeof(char) * (strlen(command) + 1));
        assert(cmd_copy != NULL);
        strncpy(cmd_copy, command, (strlen(command) + 1));
        num_of_commands++;

        if (pipe_flag == 1)
        { //pipe flag is on and there is '|' in the command

            char **pipe_args = split_pipe(cmd_copy, redirection_check);
            char *left = pipe_args[0];
            char *right = pipe_args[1];
            char **left_cmd = splitCommand(left);
            if (check_redirection > 0)
            { //there is redirection in the command

                pipe_flag = 0;
                char **sub_right = split_pipe(right, redirection_check);
                char *sub_right_left = sub_right[0];             //the left side of redirec after pipe
                sub_right_right = sub_right[1];                  //the right side of redirec after pipe - file name
                char **cmd_right = splitCommand(sub_right_left); //to split the left side of the right command
                exec_pipe(left_cmd, cmd_right, sub_right_right, redirection_check);
                
                free_arguments(pipe_args, cmd_copy);
                free(left_cmd);
                free(sub_right);
                free(cmd_right);
                
                continue;
            }
            else
            {
                char **right_cmd = splitCommand(right);
                exec_pipe(left_cmd, right_cmd, NULL, redirection_check);
                pipe_flag = 0;
                free_arguments(pipe_args, cmd_copy);
                free(left_cmd);
                free(right_cmd);
                continue;
            }
        }

        if (redirection_check > 0) {
            char **redirection_only = split_pipe(cmd_copy, redirection_check);
            sub_right_right = redirection_only[1];//the file name
            args = splitCommand(redirection_only[0]);//to split the right side of the command
            exec_cmd(args, sub_right_right, redirection_check);
            free_arguments(redirection_only, cmd_copy);
            free(args);
            continue;
        }

        args = splitCommand(cmd_copy);
        if (strcmp(command, "done\n") == 0)
        {
            average = (double)total_length_of_commands / (double)num_of_commands;
            printf("Num of commands: %d\n", num_of_commands);
            printf("Total length of all commands: %d\n", total_length_of_commands);
            printf("Average length of all commands: %lf\n", average);
            printf("See you Next time!\n");
            free_arguments(args, cmd_copy);
            break;
        }
        else if (strcmp(command, "cd\n") == 0)
        {
            printf("Comand not supperted (Yet)\n");
            free_arguments(args, cmd_copy);
            continue;
        }
        else
        {
            exec_cmd(args, NULL, redirection_check);
        }
        free_arguments(args, cmd_copy);
    }
}

//a function that get the path, and print it to the screen in the requested format
void getPath()
{
    struct passwd *pws;
    long size;
    char *buf;
    char *ptr;
    //uid_t uid = 0; //changs - added this line
    size = pathconf(".", _PC_PATH_MAX);
    if ((buf = (char *)malloc((size_t)size)) != NULL)
    {
        ptr = getcwd(buf, (size_t)size);
    }
    if ((pws = getpwuid(getuid())) == NULL)
    {
        perror("getpwuid() error");
    }
    else
    {
        pws = getpwuid(geteuid());
        printf("%s@%s>", pws->pw_name, ptr);
        free(buf);
    }
}

//the function split the command the user to the different words.
//the function use strtok to split the command
char **splitCommand(char *command)
{
    int args_size = SIZE_OF_ARGS, index = 0;
    char **words = (char **)malloc(args_size * sizeof(char *));
    assert(words != NULL);
    char *word;

    word = strtok(command, INDEX_FOR_DELETE); //delete the first word from command

    while (word != NULL)
    {
        words[index] = word;
        index++;
        args_size++;
        words = (char **)realloc(words, args_size * sizeof(char *));
        assert(words != NULL);

        word = strtok(NULL, INDEX_FOR_DELETE);
    }
    words[index] = NULL;
    return words;
}

//the exec funcion create another processv - child process - that would occure
//while the "father process" is happening
void exec_cmd(char **args, char *file_name, int sign)
{
    pid_t pid;
    pid = fork();
    if (pid == 0)
    {
        //child process
        redirection(file_name, sign);
        if (execvp(args[0], args) == -1)
        {
            perror("exec failed");
            exit(EXIT_FAILURE);
        }
    }
    else if (pid < 0)
    {
        //Error forking
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        //parent process
        wait(NULL);
    }
}

//this function would be called if there is a pipe char in the command
//and it split into 2 different process
//the function would also create a pipe and would route the input the the output
//of the process with dup2 function
void exec_pipe(char **left_cmd, char **right_cmd, char *file_name, int sign)
{
    pid_t left_child;
    pid_t right_child;
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1)
    {
        perror("pipe failure");
        exit(EXIT_FAILURE);
    }

    left_child = fork();
    if (left_child < 0)
    {
        //Error forking
        perror("fork_left failed");
        exit(EXIT_FAILURE);
    }
    if (left_child == 0)
    {

        int check_dup = dup2(pipe_fd[1], STDOUT_FILENO); //writing to the pipe
        if (check_dup == -1)
        {
            perror("dup failure");
            exit(EXIT_FAILURE);
        }
        close(pipe_fd[0]); //closing the reading side
        close(pipe_fd[1]); //closing the write side after writing

        if (execvp(left_cmd[0], left_cmd) == -1)
        {
            perror("exec_left failed");
            exit(EXIT_FAILURE);
            
        }
    }

    right_child = fork(); //the second child process
    if (right_child < 0)
    {
        //Error forking
        perror("fork_right failed");
        exit(EXIT_FAILURE);
    }
    if (right_child == 0)
    {

        int check_dup = dup2(pipe_fd[0], STDIN_FILENO); //reading from the pipe
        if (check_dup == -1)
        {
            perror("dup failure");
            exit(EXIT_FAILURE);
        }
        close(pipe_fd[1]); //closing the writing side
        close(pipe_fd[0]); //closing the reading side after reading

        redirection(file_name, sign);
        if (execvp(right_cmd[0], right_cmd) == -1)
        {
            perror("exec_right failed");
            exit(EXIT_FAILURE);
        }
    }
    //parent process
    close(pipe_fd[1]);
    close(pipe_fd[0]);

    wait(NULL);
    wait(NULL);
}

//function that callad to free the arguments which alocated on the heap
void free_arguments(char **args, char *cmd_copy)
{
    free(cmd_copy);
    free(args);
}

//to search for the pipe char in the command.
//if there was pipe, the global flag would turn on
void search_pipe_in_command(char *cmd)
{
    int i;
    for (i = 0; i < strlen(cmd) + 1; i++)
    {
        if (cmd[i] == '|')
        {
            pipe_flag = 1;
        }
    }
}

//the function return char** array with 2 cells, the first is for the left
//side of the command - left to the pipe, and in the other cell the right
//side of the pipe, without the redirection sign
/////despite the name of split pipe, i'm using this exec function
//even if there is no pipe in the command and there is only redirection sign
//in the typed command
char **split_pipe(char *cmd, int sign)
{
    int position = 0;
    char **command = (char **)malloc(sizeof(char *) * ARGS_SIZE);
    assert(command != NULL);
    char *temp;
    if (pipe_flag == 1 || sign == -1)
    {
        temp = strtok(cmd, PIPE_CHARS);
        while (temp != NULL)
        {
            command[position] = temp;
            position++;
            temp = strtok(NULL, PIPE_CHARS);
        }
    }
    else if (pipe_flag == 0 && sign == 1)
    { //1 = FILE_OUT
        temp = strtok(cmd, FILE_OUT);
        while (temp != NULL)
        {
            command[position] = temp;
            position++;
            temp = strtok(NULL, FILE_OUT);
        }
    }

    else if (pipe_flag == 0 && sign == 2)
    { //2 = FILE_APPEND
        temp = strtok(cmd, FILE_OUT_APPEND);
        while (temp != NULL)
        {
            command[position] = temp;
            position++;
            temp = strtok(NULL, FILE_OUT_APPEND);
        }
    }

    else if (pipe_flag == 0 && sign == 3)
    { //3 = FILE_ERROR
        temp = strtok(cmd, FILE_ERR);
        while (temp != NULL)
        {
            command[position] = temp;
            position++;
            temp = strtok(NULL, FILE_ERR);
        }
    }

    else if (pipe_flag == 0 && sign == 4)
    { //4 = FILE_IN
        temp = strtok(cmd, FILE_IN);
        while (temp != NULL)
        {
            command[position] = temp;
            position++;
            temp = strtok(NULL, FILE_IN);
        }
    }

    return command;
}

//function to redirect and handle the file path
//according to the sign we received which sign the different redirections
void redirection(char *file_name, int sign)
{
    int fd;
    int check_dup = 0;
    if (sign > 0)
    {

        file_name = strtok(file_name, " \n"); //remove the spaces in the file name

        if (sign == 1)
        { //file out
            fd = open(file_name, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);
            check_dup = dup2(fd, STDOUT_FILENO);

            if (check_dup == -1)
            {
                perror("dup failed");
                exit(1);
            }
            close(fd);
        }
        else if (sign == 2)
        { //file out append
            fd = open(file_name, O_CREAT | O_APPEND | O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);
            check_dup = dup2(fd, STDOUT_FILENO);
            if (check_dup == -1)
            {
                perror("dup failed");
                exit(1);
            }
            close(fd);
        }

        else if (sign == 3)
        {
            printf("file err ");
            fd = open(file_name, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);
            check_dup = dup2(fd, STDERR_FILENO);

            if (check_dup == -1)
            {
                perror("dup failed");
                exit(1);
            }
            close(fd);
        }
        else if (sign == 4)
        {
            fd = open(file_name, O_RDONLY, 0);
            check_dup = dup2(fd, STDIN_FILENO);
            if (check_dup == -1)
            {
                perror("dup failed");
                exit(1);
            }
            close(fd);
        }
    }
}

//the funcion would get the command and would return
int check_redirection(char *buffer)
{
    char *ptr;
    if ((ptr = strchr(buffer, '>')) != NULL)
    {
        if (*(++ptr) == '>')
        {
            return 2;
        }
        --ptr;
        if (*(--ptr) == '2')
        {
            return 3;
        }
        return 1;
    }

    if ((ptr = strchr(buffer, '<')) != NULL)
    {
        return 4;
    }
    // if no redirection return -1;
    return -1;
}
