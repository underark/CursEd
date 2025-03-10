#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char* string = malloc(sizeof(char));
    if (!string)
    {
        printf("Memory allocation failed\n");
        return 1;
    }
    char* add_string = "hello";

    int cursor = 0;
    int size = 1;

    for (int i = 0; add_string[i] != '\0'; i++ )
    {
        string = realloc(string, sizeof(char));
        if (!string)
        {
            printf("Memory reallocation failed\n");
            return 1;
        }
        string[cursor] = add_string[i];
        cursor++;
        size++;
    }

    cursor = 2;
    string = realloc(string, sizeof(char));
    if (!string)
    {
        printf("Memory reallocation failed\n");
        return 1;
    }
    size++;

    for (int i = size - 1; i >= cursor; i--)
    {
        string[i] = string[i - 1];
    }

    string[cursor] = 'n';

    printf("%s\n", string);
    free(string);
}