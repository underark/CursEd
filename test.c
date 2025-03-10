#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char* first_string = malloc(sizeof(char) * 100);
    char* second_string = malloc(sizeof(char) * 100);

    first_string[0] = 'a';
    first_string[1] = 'b';
    char* memory_location = &second_string[50];

    memmove(memory_location, first_string, 2);

    printf("%c %c\n", second_string[50], second_string[51]);

    free(first_string);
    free(second_string);
    return 0;
}