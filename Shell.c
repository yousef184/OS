#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>

//variables used in export
char *variables[100];
char *values[100];
int var_count = 0;

void write_to_log_file()
{
    FILE *log = fopen("lab.log","a"); //open a file in append mode
   
    fprintf(log, "%s\n", "Child terminated"); //writing child terminated to log file
    fclose(log);
}




void setup_environment(){
    char cwd[100]; 
    getcwd(cwd, sizeof(cwd)); //getting the current working directory
    chdir(cwd); //cd to the current working directory
}


// Change directory
void change_directory(char *path) {
    if (path == NULL || strcmp(path, "~") == 0) { //cases to redirect to home
        path = getenv("HOME");
    }

    chdir(path); //change directory 
   
}

// Handle export command
void handle_export(char *assignment) {
    char *var_name = strtok(assignment, "="); //first get the variable name before `=`
    char *var_value = strtok(NULL, ""); // then get it's value after `=`

    for (int i = 0; i < var_count; i++) { //itterate over all the array
        if (strcmp(variables[i], var_name) == 0) { //check if the variable already exists
            values[i] = strdup(var_value); // if it does update it's value
            return;
        }
    }

    variables[var_count] = strdup(var_name); //store new variable
    values[var_count] = strdup(var_value); // store it's value
    var_count++; //increase variable count
}

void expand_variables(char **args) {
    for (int i = 0; args[i] != NULL; i++) { //iterate over args
        if (args[i][0] == '$') {  // if argument starts with '$'
            char *var_name = args[i] + 1;  // remove $ from the name

            // find the variable in the stored list
            for (int j = 0; j < var_count; j++) {
                if (strcmp(var_name, variables[j]) == 0) { //if found
                    args[i] = strdup(values[j]);  // replace it with it's value
                    break;
                }
            }
        }
    }
}


void handle_echo(char **args) {
    char *str = args[1];
    if (str[0] == '"' && str[strlen(str) - 1] == '"') { //if quotes are present
        str++; // Remove first quote
        str[strlen(str) - 1] = '\0'; // Remove last quote
    }

    for (int i = 0; i < var_count; i++) { 
        if (strstr(str, variables[i])) {//checking in case of stored variable
            printf("%s\n", values[i]); //substituting the variable with it's value
            return;
        }
    }

    printf("%s\n", str); //else printing the whole string
}



// execute built-in commands
void execute_shell_bultin(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        change_directory(args[1]);
    } else if (strcmp(args[0], "echo") == 0) {
        handle_echo(args);
    } else if (strcmp(args[0], "export") == 0) {
        if (args[1] != NULL) {
            handle_export(args[1]);
        } else {
            fprintf(stderr, "export: missing argument\n");
        }
    }
}



void execute_command(char **args, int is_background){
    expand_variables(args);
    pid_t child_id = fork(); //crate new child process to execute the command
    if (child_id == 0) //child process
    {
        execvp(args[0], args);
        
    }
        else { // Parent process
            if (!is_background) { //if not a backgorund process wait till it finishes
                int status;
                waitpid(child_id, &status, 0);
 
            } else { //else run it in background and continue
                printf("[Background PID: %d]\n", child_id);
            }
        }
    
}
void parse_input(char *input, char **args, int *bg)
{
    int i =0;
    char *token = strtok(input, " "); //split the string into parts based on " " as delimter
    while (token != NULL) { //if there is an argument
        if (strcmp(token, "&") == 0) { //check first if the argument is &
            *bg = 1; //set the flag to 1
        } else {
            args[i++] = token; // append token to the args array 
        }
        token = strtok(NULL, " "); //return the next token or argument
    }
    args[i] = NULL; //if no arguments present
}
void shell(){
    char input[100];
    char *args[100];
    int bg =0;
    while (1)
    {
        fgets(input, 100, stdin); //get input
        input[strcspn(input, "\n")] = '\0'; //remove new line
        if(strlen(input)==0) //ignore empty input
        {
            continue;
        }
        parse_input(input, args, &bg); //parse the input by splitiing it
        if (strcmp(args[0], "exit") == 0) { //if command is exit 
            exit(0);
        } else if (strcmp(args[0], "cd") == 0 || strcmp(args[0], "echo") == 0 || strcmp(args[0], "export") == 0) { // if command is a built in command
            execute_shell_bultin(args);
        } else { //if other commands
            execute_command(args, bg);
        }
    }
}

void on_child_exit(int signum) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) { // clean up the child and prevent zombie process 
        write_to_log_file(); //append to log file child terminated
    }
}

void main(){
    signal (SIGCHLD, on_child_exit); //reap zombie processes 
    setup_environment();
    shell();
}
