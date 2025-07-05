#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include "parser.h"
/**
 * Simple shell implementation in C, akin to the shells found in modern
 * computers. It supports basic command execution, background tasks,
 * and a history feature. The shell keeps track of the last 10 commands and
 * allows users to re-execute them. It also handles some signals and
 * avoids creating zombie processes.
 * 
*/

__author__ = "https://github.com/theredplanetsings"
__date__ = "25/04/2024"

#define MAX_CMD_LENGTH 1000
#define HISTORY_LENGTH 10
/**
 * This struct is used to store the details of a command that has been
 * executed in the shell. It includes the command itself as a
 * string and a unique command ID.
 * @var command[MAX_CMD_LENGTH] The command that was executed.
 * @var commandID The unique ID of the command.
 */
typedef struct {
    char command[MAX_CMD_LENGTH];
    unsigned int commandID;
} history_t;

history_t history[HISTORY_LENGTH];
// starts at 1 for user-friendly command IDs
unsigned int currentCommandID = 1;
// separates the index for the history array
unsigned int historyIndex = 0;
/**
 * This function adds a command to the history array. It assigns the current command
 * ID to the command, copies the command string into the history, and then
 * increments the current command ID. If the history is full, it
 * overwrites the oldest command.
 * @param command The command to add to the history.
 */
void add_to_history(char* command) {
    // ignores commands that are just whitespace
    char* c;
    for (c = command; *c != '\0'; c++) {
        // if character != space, breaks loop and adds to history
        if (!isspace((unsigned char)*c)) { break; }
    }
    // if command is whitespace, returns and adds nothing to history
    if (*c == '\0') { return; }
    // copies command to the 'command' portion of current history item
    strncpy(history[historyIndex].command, command, MAX_CMD_LENGTH - 1);
    // adds null character to end of the command to ensure it's a valid string
    history[historyIndex].command[MAX_CMD_LENGTH - 1] = '\0';
    // assigns current command ID to the 'commandID' portion of current history item
    //then increments current commandID for the next command
    history[historyIndex].commandID = currentCommandID++;
    // moves history index to next position, wrapping it around to the start
    // if it reaches the end
    historyIndex = (historyIndex + 1) % HISTORY_LENGTH;
}
/**
 * This function iterates over the history array and prints each command along with its ID.
 * The commands are printed in the order they were added, with the oldest command first.
 * Commands with an ID of 0 (which indicates they are not valid commands) are skipped.
 */
void print_history() {
    // iterates through history items
    for (int i = 0; i < HISTORY_LENGTH; i++) {
        // calculates index of current item, wrapping around to start if it reaches the end
        int index = (historyIndex + i) % HISTORY_LENGTH;
        // if "command" portion is not an empty string,
        // prints the commandID + command
        if (history[index].command[0] != '\0') {
            printf("%d %s ", history[index].commandID, history[index].command);
            // flushes output buffer to ensure command is immediately printed
            fflush(stdout);
        }
    }// prints newline character at end of history
    printf("\n");
    // flushes output buffer to ensure newline char is immediately printed
    fflush(stdout);
}
/**
 * This function takes a command and executes it. If the command is "exit",
 * it terminates the program. If the command is "history", it
 * prints the command history. For other commands, it forks a child process
 * and uses execvp to execute the command. If the command is not found,
 * it prints an error message and returns
 * EXIT_FAILURE. If the command is a background command,
 * the function returns immediately without waiting for
 * the child process to finish. If the command is not a
 * background command, the function waits for the child
 * process to finish before returning.
 * 
 * @param command The command to execute.
 * @param background A flag indicating whether the command is a background command.
 * @param validCommand A pointer to an int where the function will
 * store a flag indicating whether the command was valid.
 * @return EXIT_SUCCESS if the command was executed successfully,
 * EXIT_FAILURE otherwise.
 */
int execute_command(char** command, int background, int* validCommand) {
    // if command is null/empty string, returns immediately with "success" status
    if (command[0] == NULL || command[0][0] == '\0') { return EXIT_SUCCESS; }
    // creates a "pipe" for communication from parent to child processes
    int pipefd[2]; // [0] is read, [1] is write
    pipe(pipefd);
    // initializes variable to track if command execution fails or not
    int failure = 0; // initialize failure to 0
    // if command is "exit", exits the program
    if (command[0] != NULL && strcmp(command[0], "exit") == 0) {
        exit(0);
    // if command is "history", prints history list and returns w/ "success" stats
    } else if (command[0] != NULL && strcmp(command[0], "history") == 0) {
        print_history();
        *validCommand = 1;
        return EXIT_SUCCESS;
    // if command is neither "exit" nor "history, forks a child process to execute the command
    } else {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        // in the child process, executes the command
        // child process ID == 0
        } else if (pid == 0) {
            // close the read end of the pipe in child
            close(pipefd[0]);
            // the child process
            if (execvp(command[0], command) == -1) {
                // execvp returns -1 if the command is not found and prints error message
                printf("%s: command not found", command[0]);
                fflush(stdout);
                //sets failure to 1
                failure = 1;
                // write the failure status to pipe
                write(pipefd[1], &failure, sizeof(failure));
                // closes the write end of pipe
                close(pipefd[1]);
                // exits to prevent continues code execution by the child process
                // ie to prevent child taking on role of the parent process
                exit(EXIT_FAILURE);
            }
            // if execvp succeeds, overwrites the failure status in the pipe
            //write(pipefd[1], &failure, sizeof(failure));
            // closes the write end of the pipe
            //close(pipefd[1]);
            // in parent process, waits for child process to finish
            // if the command isn't a background command

            // parent process here bc PID != 0, which is the child's ID
        } else {
            if (!background) {
                int status;
                // waitpid tells parent to wait for child process to finish running
                // before it continues itself. 0 is the ID of the child
                waitpid(pid, &status, 0);
                // the parent process waits for the child
                // if the command is not a background command
                // close write end of the pipe within the parent
                close(pipefd[1]);
                // read failure status from pipe
                read(pipefd[0], &failure, sizeof(failure));
                // close read end of pipe
                close(pipefd[0]);
                // if failure occurs, exits with failure status
                if (failure) {
                    *validCommand = 0;
                    return EXIT_FAILURE; //exit s
                }
            }
        }
    }
    return EXIT_SUCCESS;
}
/**
 * This function runs an infinite loop that reads commands
 * from the user, parses them, and executes them.
 * It handles special commands like "exit" and "history" directly.
 * For other commands, it forks a child process
 * and uses execvp to execute the command. If the command
 * starts with "!", it treats it as a request to re-execute
 * a command from the history. After each command is executed,
 * it is added to the history.
 * The function also handles memory deallocation for the parsed command
 * and ignores SIGCHLD signals to automatically reap child processes.
 * @return 0 when the program is terminated.
 */
int main() {
    // initializes history with empty commands
    for (int i = 0; i < HISTORY_LENGTH; i++) {
        history[i].commandID = 0;
        history[i].command[0] = '\0';
    }
    // sets SIGCHLD signal to be ignored to prevent zombie processes
    signal(SIGCHLD, SIG_IGN);
    // enters loop where it reads & executes commands until "exit" is entered
    char command[MAX_CMD_LENGTH];
    while (1) {
        // prints shell prompt & flushes the output buffer to ensure it is instantly printed
        printf("catshell> ");
        fflush(stdout);
        // reads command from user with fgets()
        fgets(command, MAX_CMD_LENGTH - 1, stdin);
        command[strcspn(command, "\n")] = '\0';
        // parses the command & checks if it should be run in the background or not
        int background = 0;
        char** parsedCommand = parseCommand(command, &background);
        // initialises a variable to track if command is valid or not
        int validCommand = 1;
        // if command is "exit", frees the memory allocated for the command and breaks loop (exits)
        if (parsedCommand[0] != NULL && strcmp(parsedCommand[0], "exit") == 0) {
            for (int i = 0; parsedCommand[i] != NULL; i++) {
                free(parsedCommand[i]);
            }
            free(parsedCommand);
            break;
        // if command is "history", adds the command to histroy and prints history list
        } else if (parsedCommand[0] != NULL && strcmp(parsedCommand[0], "history") == 0) {
            add_to_history(command);
            print_history();
        // if command begins with "!", treats it as a command to execute
        // the associated commandID from history
        } else if (parsedCommand[0] != NULL && parsedCommand[0][0] == '!') {
            // extracts commandID from the command
            int commandID = atoi(parsedCommand[0] + 1);
            // if commandID is invalid, prints error message
            if (commandID < 1 || commandID >= currentCommandID) {
                printf("%s: event not found", parsedCommand[0]);
                fflush(stdout);
            // runs if commandID is in fact valid
            } else {
                // searches history for command with the given ID
                int found = 0;
                for (int i = 0; i < HISTORY_LENGTH; i++) {
                    int index = (historyIndex + i) % HISTORY_LENGTH;
                    // if command is found in history
                    if (history[index].commandID == commandID) {
                        // it is then parsed, added to history, and executed
                        char** historyCommand = parseCommand(history[index].command, &background);
                        add_to_history(history[index].command);
                        execute_command(historyCommand, background, &validCommand);
                        found = 1;
                        // frees the memory allocated by parseCommand for the history command
                        for (int j = 0; historyCommand[j] != NULL; j++) {
                            free(historyCommand[j]);
                        }
                        free(historyCommand);
                        break;
                    }
                }
                // if command is not found, prints error message
                if (!found) {
                    printf("%s: event not found", parsedCommand[0]);
                    fflush(stdout);
                }
            }
        // if command is anything else, executes it and adds it to the history
        } else {
            execute_command(parsedCommand, background, &validCommand);
            add_to_history(command);
        }
        // frees memory allocated for the parsed command
        for (int i = 0; parsedCommand[i] != NULL; i++) {
            free(parsedCommand[i]);
        }
        free(parsedCommand);
    }
    // returns 0, indicating successful program execution
    return 0;
}
