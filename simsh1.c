//
// Created by Adam Mitchell on 2/19/16.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/errno.h>
#include <signal.h>

//*************************************************************************************
typedef struct {
    char ** tokens;           //pointer to "num_tokens" null-terminated strings
    unsigned int num_tokens;  //size of "tokens" string pointer array
} chopped_line_t ;

chopped_line_t * get_chopped_line( const char * iline );
void free_chopped_line( chopped_line_t * icl );

//******************* List Stuff ********************
//struct list_node_t{
//    int val;
//    struct list_node_t * next;
//};
//
//typedef struct {
//    struct list_node_t * head;
//} list_t;
//
//list_t * list_create( void );
//void list_clear( list_t * ilist );
//void list_delete( list_t * ilist );
//void list_insert_val( list_t * ilist, int i );
//void list_remove_val( list_t * ilist, int i );
//**************************************************************************************

typedef struct {
    char ** args;           //pointer to "args" null-terminated strings
    int num_args;           //size of "args" string pointer array
} program_with_args_t ;

int MAX_ARGS = 32;

int valid(chopped_line_t *args);
program_with_args_t** construct_programs(chopped_line_t *parsed_line);
bool check_exit( char * line );


int main (int argc, char **argv)
{
    size_t buffer_size = 4096;
    char *input_buffer = malloc(buffer_size);
    chopped_line_t *parsed_command;
    program_with_args_t ** programs;

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
    while (true)
    {
        printf("simsh: ");

        bytes_read = getline(&input_buffer, &buffer_size, stdin);

        if (!strcmp(input_buffer,"\n"))
        {
            continue;
        }

        if (bytes_read == -1 || check_exit(input_buffer))
        {
            printf("exiting \n");
            exit(1);
        }

        // parse the command arguments into an array of arrays
        parsed_command = get_chopped_line(input_buffer);

        int valid_input = valid(parsed_command);
        if (valid_input == -2)
        {
            printf("operator & must appear at end of command line\n");
            continue;
        }
        if (valid_input == -1)
        {
            printf("invalid input\n");
            continue;
        }

        // input is valid now create child process to run program
        // fork to create child process
        pid = fork ();
        if (pid == -1)
        {
            printf( "\"fork\" failed\n" );
            continue;
        }

        if (pid != 0)
        { // parent process
            printf ( "Parent process\n" );
            int status;
            if (valid_input != 2)
            { // user didn't type & must wait on child process
                waitpid(pid, &status, 0);
            }
        }
        else
        {// child process
            printf ( "Child process\n" );

            programs = construct_programs(parsed_command);
            free_chopped_line(parsed_command);

            printf ( "command: %s\n", programs[0]->args[0]);
            execvp(programs[0]->args[0], programs[0]->args);
            _exit(1);
        }

    }
}

program_with_args_t** construct_programs(chopped_line_t *parsed_line)
{
    int i, num_processes_needed = 1;
    program_with_args_t ** programs;

    for(i = 0; i < parsed_line->num_tokens; i++)
    {
        if(!strcmp(parsed_line->tokens[i],"|"))
        {
            num_processes_needed++;
        }
    }

    programs = (program_with_args_t **) malloc (num_processes_needed * sizeof(chopped_line_t) );


    for (i = 0; i < num_processes_needed; i++)
    {
        programs[i] = (program_with_args_t *) calloc( 1, sizeof(chopped_line_t));
        programs[i]->args = ( char ** ) malloc(MAX_ARGS * MAX_ARGS * sizeof( char * ) );
        programs[i]->num_args = 0;
    }

    int process_index = 0;
    bool have_name = false;

    for (i = 0; i < parsed_line->num_tokens; i++)
    {
        char *current_token = parsed_line->tokens[i];
        if(!strcmp(current_token,"|"))
        {
            process_index++;
            have_name = false;
            continue;
        }
        else if(!strcmp(current_token,">") || !strcmp(current_token,"<") || !strcmp(current_token,"<<") || !strcmp(current_token,"&"))
        {
            continue;
        }
        else if (!have_name)
        { // haven't found index's name
            programs[process_index]->args[0] = malloc(strlen(current_token));
            programs[process_index]->args[0] = strdup (current_token);
            programs[process_index]->num_args = 1;

            have_name = true;
            continue;
        }
        else
        { // have found index's name
            int args_array_index = programs[process_index]->num_args;
            programs[process_index]->args[args_array_index] = ( char * ) malloc(strlen(current_token)+1 );
            programs[process_index]->args[args_array_index] = strdup (current_token);
            programs[process_index]->num_args = args_array_index+1;
        }
    }
    return programs;
}


int valid(chopped_line_t *args)
{
    int i, num_args = args->num_tokens, num_inputs = 0, num_outputs = 0, num_ands = 0;

    for(i = 0; i < num_args; i++)
    {
        char *current_token = args->tokens[i];
        if (num_ands > 0)
        {
            return -2;
        }
        if(!strcmp(current_token,"&"))
        {
            num_ands++;
        }
        if(!strcmp(current_token,"<"))
        {
            num_inputs++;
        }
        if(!strcmp(current_token,">>") || !strcmp(current_token,">"))
        {
            num_outputs++;
        }
    }

    if (num_inputs > 1 || num_outputs > 1)
    {
        return -1;
    }
    if (num_ands == 1)
    {// there is a & in the command line so don't wait for child.
        return 2;
    }
    // no & in command line must wait on child.
    return 1;
}

bool check_exit( char * line )
{
    chopped_line_t *parsed = get_chopped_line(line);
    if (!strcmp(parsed->tokens[0],"exit"))
    {
        return true;
    }
    return false;
}


void handle_sigchld(int sig) {
    int saved_errno = errno;
    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
    errno = saved_errno;
}

//******************************************************************************
chopped_line_t * get_chopped_line( const char * iline )
{
    chopped_line_t * cl;
    char * line_copy;
    const char * delim = " \t\n";
    char * cur_token;

    cl = (chopped_line_t *) malloc ( sizeof(chopped_line_t) );
    cl->tokens = NULL;
    cl->num_tokens = 0;

    if( iline == NULL )
        return cl;

    line_copy = strdup( iline );
    cur_token = strtok( line_copy, delim );
    if( cur_token == NULL )
        return cl;

    do {
        cl->num_tokens++;
        cl->tokens = ( char ** ) realloc( cl->tokens,
                                          cl->num_tokens * sizeof( char * ) );
        cl->tokens[ cl->num_tokens - 1 ] = cur_token;
    } while((cur_token = strtok(NULL, delim)));

    return cl;
}

void free_chopped_line( chopped_line_t * icl )
{
    if( icl == NULL )
        return;

    free(icl->tokens);
    free(icl);
}

//******************* List Stuff ********************
//list_t * list_create( void )
//{
//    list_t * new_list = ( list_t * )malloc(sizeof(list_t));
//    new_list->head = NULL;
//    return new_list;
//}
//
//void list_clear( list_t * ilist )
//{
//    struct list_node_t * cur_node, * tmp_node;
//
//    cur_node = ilist->head;
//    while( cur_node != NULL ) {
//        tmp_node = cur_node;
//        cur_node = cur_node->next;
//        free( tmp_node );
//    }
//    ilist->head = NULL;
//}
//
//void list_delete( list_t * ilist )
//{
//    struct list_node_t * cur_node, * tmp_node;
//
//    cur_node = ilist->head;
//    while( cur_node != NULL ) {
//        tmp_node = cur_node;
//        cur_node = cur_node->next;
//        free( tmp_node );
//    }
//    free( ilist );
//}
//
//void list_insert_val( list_t * ilist, int i )
//{
//    struct list_node_t * new_node = (struct list_node_t *)
//            malloc(sizeof(struct list_node_t));
//    new_node->val = i;
//    new_node->next = ilist->head;
//    ilist->head = new_node;
//}
//
//void list_remove_val( list_t * ilist, int i )
//{
//    struct list_node_t * prev_node, *cur_node;
//
//    prev_node = NULL;
//    cur_node = ilist->head;
//    while( cur_node != NULL ) {
//        if( cur_node->val == i ) {
//            if( prev_node == NULL ) {
//                ilist->head = cur_node->next;
//            }
//            else{
//                prev_node->next = cur_node->next;
//            }
//            free( cur_node );
//            return;
//        }
//        prev_node = cur_node;
//        cur_node = cur_node->next;
//    }
//}
//*********************************************************************************