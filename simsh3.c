//
// Created by Adam Mitchell on 2/19/16.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include "chop_line.h"

typedef struct {
    char **args;           //pointer to "args" null-terminated strings
    char *infile;           //name of infile
    char *outfile;          //name of outfile
    int num_args;           //size of "args" string pointer array
    int append;
    int ampersand;
    int read_pipe;
    int open_pipe;
} program_with_args_t;

int MAX_ARGS = 32;

int valid(chopped_line_t *args);

program_with_args_t **construct_programs(chopped_line_t *parsed_line);

bool check_exit(char *line);

void handle_sigchld(int sig);

int run_command(program_with_args_t *programs);


int main(int argc, char **argv) {
    size_t buffer_size = 4096;
    char *input_buffer = malloc(buffer_size);
    chopped_line_t *parsed_command;
    program_with_args_t **programs = NULL;

    // Register signal handlers
    struct sigaction action;
    action.sa_handler = &handle_sigchld;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &action, 0) == -1) {
        perror(0);
        exit(1);
    }

    int pid;
    ssize_t bytes_read = 0;
    while (true) {
        printf("simsh: ");

        bytes_read = getline(&input_buffer, &buffer_size, stdin);

        if (!strcmp(input_buffer, "\n")) {
            continue;
        }

        if (bytes_read == -1 || check_exit(input_buffer)) {
            printf("exiting...\n");
            exit(1);
        }

        // parse the command arguments into an array of arrays
        parsed_command = get_chopped_line(input_buffer);

        int valid_input = valid(parsed_command);

        if (valid_input == -1) {
            continue;
        }

        // build programs array
        programs = construct_programs(parsed_command);

        while (*programs != NULL) {
            if (run_command(*programs))
                programs++;
            else
                perror("Error");
        }
    }
}

int run_command(program_with_args_t *programs) {
    int pid;
    int pipefd[2];

    // input is valid now create child process to run program
    // fork to create child process
    pid = fork();
    if (pid == -1) {
        printf("\"fork\" failed\n");
        return 1;
    }

    if (pid != 0) { // parent process
        int status;
        if (programs->ampersand == -1) { // user didn't type & must wait on child process
            waitpid(pid, &status, 0);
        }
        return 1;
    }
    else {// child process
        int infile_descr, outfile_descr = 0, err;

        if (programs->infile != NULL) // if <
        {
            if (access(programs->infile, F_OK) != -1) { // file exists
                infile_descr = open(programs->infile, O_RDONLY);
                dup2(infile_descr, STDIN_FILENO); // redirect std in to new file
                close(infile_descr);
            }
            else { // file doesn't exist
                printf("%s: No such file or directory.\n", programs->infile);
                return 1;
            }
        }

        if (programs->outfile != NULL) // if >> or >
        {
            if (programs->append == 0) { // don't append to file i.e >
                if (access(programs->outfile, F_OK) != -1) { // file exists
                    printf("%s: File exists.\n", programs->outfile);
                    return 1;
                }
                else {
                    outfile_descr = open(programs->outfile, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
                    if (outfile_descr == -1) {
                        perror(programs->outfile);
                        exit(1);
                    }
                }
            }
            else if (programs->append == 1) { // append to file i.e >>

                outfile_descr = open(programs->outfile, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
                if (outfile_descr == -1) {
                    perror(programs->outfile);
                    exit(1);
                }
            }
            err = dup2(outfile_descr, STDOUT_FILENO); // redirect std out to new file
            if (err == -1) {
                perror(programs->outfile);
                exit(1);
            }
            close(outfile_descr);
        }

        if (programs->open_pipe == 1)
        {
            pipe(pipefd);

            // replace stdout with write part of pipe
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);
        }

        if (programs->read_pipe == 1)
        {
            // replace stdin with read part of pipe
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);
        }

        execvp(programs->args[0], programs->args);
        printf("using outfile %s: .\n", programs->outfile);

        perror(programs->args[0]);
        exit(1);
    }
}

program_with_args_t **construct_programs(chopped_line_t *parsed_line) {
    int i, num_processes_needed = 1;
    char *last_token_was = "";
    program_with_args_t **programs;

    int num_tokens = parsed_line->num_tokens;
    char tokens[4096][32];
    for (i = 0; i < num_tokens; i++){
        strcpy( tokens[i], parsed_line->tokens[i] );
    }

    /**
     * Memory Allocation for (** programs)
     */
    for (i = 0; i < num_tokens; i++) {
        if (!strcmp(tokens[i], "|")) {
            num_processes_needed++;
        }
    }
    programs = (program_with_args_t **) malloc(num_processes_needed * sizeof(chopped_line_t));
    for (i = 0; i < num_processes_needed; i++) {
        programs[i] = (program_with_args_t *) calloc(1, sizeof(chopped_line_t));
        programs[i]->args = (char **) malloc(MAX_ARGS * MAX_ARGS * sizeof(char *));
        programs[i]->infile = NULL;
        programs[i]->outfile = NULL;
        programs[i]->num_args = 0;
        programs[i]->append = -1;
        programs[i]->ampersand = -1;
        programs[i]->open_pipe = -1;
        programs[i]->read_pipe = -1;
    }

    int process_index = 0;
    int args_array_index = 0;
    bool have_name = false;

    for (i = 0; i < num_tokens; i++) {
        char *current_token = tokens[i];

        if (!strcmp(last_token_was, "<")) {
            programs[process_index]->infile = strdup(current_token);
        }
        else if (!strcmp(last_token_was, ">")) {
            programs[process_index]->outfile = strdup(current_token);
            programs[process_index]->append = 0;
        }
        else if (!strcmp(last_token_was, ">>")) {
            programs[process_index]->outfile = strdup(current_token);
            programs[process_index]->append = 1;
        }
        else if (!strcmp(current_token, "|")) {
            process_index++;
            have_name = false;
            continue;
        }
        else if (!strcmp(current_token, "<")) {
            last_token_was = strdup(current_token);
            continue;
        }
        else if (!strcmp(current_token, ">")) {
            last_token_was = strdup(current_token);
            continue;
        }
        else if (!strcmp(current_token, ">>")) {
            last_token_was = strdup(current_token);
            continue;
        }
        else if (!strcmp(current_token, "&")) {
            programs[process_index]->ampersand = 1;
            continue;
        }
        else if (!have_name) { // haven't found index's name
            programs[process_index]->args[0] = strdup(current_token);
            programs[process_index]->args[0+1] = NULL;
            programs[process_index]->num_args = 1;

            have_name = true;
            continue;
        }
        else { // have found index's name
            args_array_index = programs[process_index]->num_args;
            programs[process_index]->args[args_array_index] = strdup(current_token);
            programs[process_index]->args[args_array_index+1] = NULL;
            programs[process_index]->num_args = args_array_index + 1;
        }
    }

    process_index = 0;
    for (i = 0; i < num_tokens; i++) {
        char *current_token = tokens[i];

        if (!strcmp(current_token, "|")) {
            process_index++;
        }
        if (!strcmp(current_token, "|")) {
            // set the previous program as the open end
            programs[process_index-1]->open_pipe = 1;
            // set the next program as the read end
            programs[process_index]->read_pipe = 1;
        }
    }
    return programs;
}


int valid(chopped_line_t *args) {
    int i, num_args = args->num_tokens, num_inputs = 0, num_outputs = 0, num_pipes = 0, num_ands = 0, last_arg = 1;

    if (!strcmp(args->tokens[0], ">") || !strcmp(args->tokens[0], ">>") || !strcmp(args->tokens[0], "<") ||
        !strcmp(args->tokens[0], "|")) {
        printf("Invalid null command.\n");
        return -1;
    }

    for (i = 0; i < num_args; i++) {
        char *current_token = args->tokens[i];

        if (i == 0 && (!strcmp(current_token, ">") || !strcmp(current_token, ">>"))) {
            printf("Invalid null command.\n");
            return -1;
        }
        else if (num_ands > 0) {
            printf("operator & must appear at end of command line\n");
            return -1;
        }

        else if (!strcmp(current_token, "&")) {
            last_arg = 1;
            num_ands++;
        }
        else if (!strcmp(current_token, "|")) {
            last_arg = -2;
            num_pipes++;
        }
        else if (!strcmp(current_token, "<")) {
            last_arg = -1;
            num_inputs++;
        }
        else if (!strcmp(current_token, ">>") || !strcmp(current_token, ">")) {
            last_arg = -1;
            num_outputs++;
        }
        else {
            last_arg = 1;
        }
    }

    if (num_inputs > 1) {
        printf("Ambiguous input redirect.\n");
        return -1;
    }
    if (num_outputs > 1) {
        printf("Ambiguous output redirect.\n");
        return -1;
    }
    if (num_ands > 1) {
        printf("operator & must appear at end of command line\n");
        return -1;
    }
    if (i > 1) {
        if ((!strcmp(args->tokens[i - 1], "&") && !strcmp(args->tokens[i - 2], "<")) ||
            (!strcmp(args->tokens[i - 1], "&") && !strcmp(args->tokens[i - 2], ">>")) ||
            (!strcmp(args->tokens[i - 1], "&") && !strcmp(args->tokens[i - 2], ">"))) {
            printf("Missing name for redirect.\n");
            return -1;
        }
    }
    if (!strcmp(args->tokens[i - 1], "<") || !strcmp(args->tokens[i - 1], ">>") || !strcmp(args->tokens[i - 1], ">")) {
        printf("Missing name for redirect.\n");
        return -1;
    }

    return 1;
}

bool check_exit(char *line) {
    chopped_line_t *parsed = get_chopped_line(line);
    if (!strcmp(parsed->tokens[0], "exit")) {
        return true;
    }
    return false;
}

void handle_sigchld(int sig) {
    int saved_errno = errno;
    while (waitpid((pid_t) (-1), 0, WNOHANG) > 0) { }
    errno = saved_errno;
}
