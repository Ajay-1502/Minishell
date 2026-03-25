#include "minishell.h"

char input_str[50];

void signal_handler(int signum)
{
    if (signum == SIGINT)
    {
        if (pid == 0)
        {
            printf("\nminishell$: ");
            fflush(stdout);
        }
        else
        {
            kill(pid, SIGINT);
        }
    }

    else if (signum == SIGTSTP)
    {
        if (pid == 0)
        {
            printf("\nminishell$: ");
            fflush(stdout);
        }
        else
        {
            kill(pid, SIGTSTP);

            stopped[stopped_count].pid = pid;
            strcpy(stopped[stopped_count].cmd, input_str);

            stopped_count++;

            printf("\nProcess Stopped\n");

            pid = 0;
        }
    }

    else if (signum == SIGCHLD)
    {
        while (waitpid(-1, &status, WNOHANG) > 0)
            ;
        pid = 0;
    }
}

// Function to read external commands from file
void extract_external_commands(char **external_commands)
{
    // Open file that contains external commands
    int fd = open("external_cmd.txt", O_RDONLY);

    if (fd == -1)
    {
        perror("Error opening external_cmd.txt");
        return;
    }

    char ch;
    char buffer[50];
    int i = 0, j = 0;

    // Read file character by character
    while (read(fd, &ch, 1) > 0)
    {
        if (ch != '\n' && ch != '\r')
        {
            buffer[j++] = ch;
        }
        else if (j > 0)
        {
            buffer[j] = '\0';
            external_commands[i] = malloc(strlen(buffer) + 1); // Allocate memory for command
            strcpy(external_commands[i], buffer);
            i++;
            j = 0;
        }
    }

    external_commands[i] = NULL; // Mark end of external commands list
    close(fd);
}

// Function to check command type
int check_command_type(char *command, char **external_commands)
{
    // Check if command is builtin command
    for (int i = 0; builtins[i] != NULL; i++)
    {
        if (strcmp(builtins[i], command) == 0)
        {
            return BUILTIN;
        }
    }

    // Check if command is external command
    for (int j = 0; external_commands[j] != NULL; j++)
    {
        if (strcmp(external_commands[j], command) == 0)
        {
            return EXTERNAL;
        }
    }

    return NO_COMMAND; // Command not found
}

// Function to extract command
char *get_command(char *input_string)
{
    char backup[100];
    // Make copy because strtok modifies string
    strcpy(backup, input_string);

    char *token = strtok(backup, " ");

    if (token != NULL)
    {
        char *command = malloc(strlen(token) + 1); // Allocate memory for command
        strcpy(command, token);
        return command;
    }

    return NULL;
}

void execute_external_command(char *input)
{
    int pipe_count = 0;
    int token_index = 0;
    int command_count = 1;

    char *args[50];

    // Tokenize input
    char *token = strtok(input, " ");

    while (token != NULL)
    {
        args[token_index] = token;
        token = strtok(NULL, " ");
        token_index++;
    }

    args[token_index] = NULL;

    int cmd_start[10];
    cmd_start[0] = 0;

    // Identify pipe positions
    for (int i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], "|") == 0)
        {
            pipe_count++;
            args[i] = NULL;

            cmd_start[command_count] = i + 1;
            command_count++;
        }
    }

    // Case 1 : No pipes
    if (pipe_count == 0)
    {
        pid = fork();

        if (pid == 0)
        {
            execvp(args[0], args);
            perror("execvp failed");
            exit(1);
        }
        else
        {
            waitpid(pid, &status, WUNTRACED);
        }
    }

    // Case 2 : Pipes present
    else
    {
        int prev_pipe[2];
        int curr_pipe[2];

        for (int i = 0; i < command_count; i++)
        {
            if (i < command_count - 1)
            {
                pipe(curr_pipe);
            }

            pid = fork();

            if (pid == 0)
            {
                // Not first command
                if (i > 0)
                {
                    dup2(prev_pipe[0], 0);
                }

                // Not last command
                if (i < command_count - 1)
                {
                    dup2(curr_pipe[1], 1);
                }

                // Close pipes
                if (i > 0)
                {
                    close(prev_pipe[0]);
                    close(prev_pipe[1]);
                }

                if (i < command_count - 1)
                {
                    close(curr_pipe[0]);
                    close(curr_pipe[1]);
                }

                execvp(args[cmd_start[i]], args + cmd_start[i]);
                perror("execvp failed");
                exit(1);
            }

            // Parent process
            if (i > 0)
            {
                close(prev_pipe[0]);
                close(prev_pipe[1]);
            }

            if (i < command_count - 1)
            {
                prev_pipe[0] = curr_pipe[0];
                prev_pipe[1] = curr_pipe[1];
            }
        }

        // Wait for all children
        for (int i = 0; i < command_count; i++)
        {
            wait(NULL);
        }
    }
}

// Function to execute builtin commands
int execute_internal_command(char *input_string)
{
    // Exit shell
    if (strcmp(input_string, "exit") == 0)
    {
        exit(0);
    }

    else if (strcmp(input_string, "pwd") == 0)
    {
        char buf[100];
        getcwd(buf, sizeof(buf)); // Get current working directory

        printf("%s\n", buf);
    }

    else if (strncmp(input_string, "cd", 2) == 0)
    {
        char *path = strtok(input_string + 3, " ");

        if (path == NULL)
        {
            path = getenv("HOME");
        }

        if (chdir(path) == 0) // Change directory
        {
            printf("Directory changed\n");
        }
        else
        {
            printf("Directory not changed\n");
        }
    }

    else if (strcmp(input_string, "jobs") == 0)
    {
        for (int i = 0; i < stopped_count; i++)
        {
            printf("[%d] %d %s\n", i + 1, stopped[i].pid, stopped[i].cmd);
        }
    }

    else if (strcmp(input_string, "fg") == 0)
    {
        if (stopped_count > 0)
        {
            pid = stopped[stopped_count - 1].pid;

            printf("%s\n", stopped[stopped_count - 1].cmd);

            kill(pid, SIGCONT);

            waitpid(pid, &status, 0);

            stopped_count--;

            pid = 0;
        }
        else
        {
            printf("No stopped jobs\n");
        }
    }

    else if (strcmp(input_string, "bg") == 0)
    {
        if (stopped_count > 0)
        {
            kill(stopped[stopped_count - 1].pid, SIGCONT);

            printf("[%d] %d running in background\n", stopped_count, stopped[stopped_count - 1].pid);

            stopped_count--;
        }
        else
        {
            printf("No stopped jobs\n");
        }
    }

    else if (strstr(input_string, "echo") != NULL)
    {
        if (strstr(input_string, "$?") != NULL)
        {
            printf("%d\n", WEXITSTATUS(status));
        }

        else if (strstr(input_string, "$$") != NULL)
        {
            printf("%d\n", getpid());
        }

        else if (strstr(input_string, "$SHELL") != NULL)
        {
            printf("%s\n", getenv("SHELL")); // Get SHELL environment variable
        }
    }

    return 0;
}

// Function to read user input
void scan_input(char *prompt, char *input)
{
    printf("%s ", prompt);

    scanf(" %[^\n]", input);

    __fpurge(stdin);
}

// Main function
int main()
{
    char prompt_str[20] = "minishell$:";

    char input_copy[50];

    char *external_commands[160];

    system("clear");

    extract_external_commands(external_commands);

    signal(SIGINT, signal_handler);
    signal(SIGTSTP, signal_handler);
    signal(SIGCHLD, signal_handler);

    while (1)
    {
        scan_input(prompt_str, input_str);

        // If "PS1=" string is present
        if (strncmp(input_str, "PS1=", 4) == 0)
        {
            if (strlen(input_str + 4) == 0)
            {
                printf("Error: Prompt cannot be empty\n");
            }

            else if (strchr(input_str + 4, ' ') != NULL)
            {
                printf("Error: Space not allowed in prompt!\n");
            }

            else
            {
                // Copy the input_str from fifth character to prompt_str
                strcpy(prompt_str, input_str + 4);
            }
        }

        // If "PS1=" string is not present
        else
        {
            strcpy(input_copy, input_str);

            char *cmd = get_command(input_str);

            int type = check_command_type(cmd, external_commands);

            free(cmd);

            // If type is EXTERNAL then execute the external commands
            if (type == EXTERNAL)
            {
                execute_external_command(input_copy);
            }

            // If type is BUILTIN then execute the internal commands
            else if (type == BUILTIN)
            {
                execute_internal_command(input_copy);
            }

            else
            {
                printf("Command not found\n");
            }
        }
    }

    return 0;
}