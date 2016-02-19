#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int validate(char **argv);


int main (int argc, char **argv)
{

    char input_buffer[4096];

    printf("simsh:: ");
    fgets(input_buffer, 4096, stdin);


    validate(argv);



}


int validate(char **argv)
{
    // program args... &, <, >, >>
    return 0;
}
