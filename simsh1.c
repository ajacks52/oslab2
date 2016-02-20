#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

int validate(char **args, int num_args);
char** parse_command(char * command_line);
char* concat(char *s1, char *s2);
char* append(char *str, char c);
int number_of_args(char * command_line);


int main (int argc, char **argv)
{

    char input_buffer[4096];
    char** parsed_command_line;
    int pid;
    while (true)
    {
        printf("simsh: ");
        fgets(input_buffer, 4096, stdin);

        // parse the command arguments into an array of arrays
        parsed_command_line = parse_command(input_buffer);

        if (!validate(parsed_command_line, number_of_args(input_buffer)))
        {
            printf("invalid input\n");
            continue;
        }



        pid = fork ();
        if (pid == -1)
        {
            printf( "\"fork\" failed\n" );
            continue;
        }

        if (pid != 0)
        { // parent process
            printf ( "Parent process" );
            int status;
            waitpid(pid, &status, 0);
        }
        else
        {
            execvp(parsed_command_line[0], (char *[]){ "./prcs1", argv[1], NULL });
            _exit(1);
        }


    }

}


char** parse_command(char * command_line)
{
    int i;
    char **parsed = NULL;
    for (i = 0; i < strlen(command_line); i++)
    {
        if (i==0)
        {
            parsed = malloc(number_of_args(command_line));
        }
        if (command_line[i] == ' ')
        {
            parsed++;
        }
        else if (command_line[i] == '\n' || command_line[i] == (char) NULL)
        {
            break;
        }
        else
        {

            *parsed = append(*parsed, command_line[i]);
        }
    }
    return parsed;
}



int validate(char **args, int num_args)
{
    int i;
    int num_inputs = 0, num_outputs = 0;


    for(i = 0; i < num_args; i++)
    {
        if(!strcmp(*args,"<"))
        {
            num_inputs++;
        }
        if (!strcmp(*args,">") || !strcmp(*args,">>"))
        {
            num_outputs++;
        }
    }

    if (num_inputs > 1 || num_outputs > 1)
    {
        return false;
    }


    return true;
}

char* append(char *str, char c)
{
    size_t len = strlen(str);
    char *str2 = malloc(len + 1 + 1 );
    strcpy(str2, str);
    str2[len] = c;
    str2[len + 1] = '\0';

    return str2;
}

int number_of_args(char * command_line)
{
    int args = 0, i;

    if (command_line != NULL)
    {
        args++;
    }
    for (i = 0; i < strlen(command_line); i++)
    {
        if (command_line[i] == ' ')
        {
            args++;
        }
    }
    return args;
}

char* concat(char *s1, char *s2)
{
    size_t len1;
    size_t len2;
    len1 = strlen(s1);
    len2 = strlen(s2);
    char *result = malloc(len1+len2+1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    memcpy(result, s1, len1);
    memcpy(result+len1, s2, len2+1);//+1 to copy the null-terminator
    return result;
}