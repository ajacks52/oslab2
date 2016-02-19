#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chop_line.h"

char* get_args(int argc, char **argv);
char* concat(char *s1, char *s2);


int main (int argc, char **argv)
{
    char *input_line;
    chopped_line_t *input_args_struct;

    char buffer[4096];

    printf("simsh:: ");
    fgets(buffer, 4096, stdin);


    validate(argv);



}


int validate(char **argv)
{
    // program args... &, <, >, >>

}
