#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct line line;
struct line
{
    int line_number;
    line* next_line;
    line* previous_line;
    char* buffer;
    char* gap_start;
    char* gap_end;
    int number_characters;
};

void addat_cursor(int input, line* current_line);
void move_right(line* current_line);
void move_left(line* current_line);
int delete(line* current_line);
line* add_line(line* document_start);
void free_lines(line* ptr);
void shuffle(line* line);

int max_x = 0;
int max_y = 0;
int x = 0;
int y = 0;

int main(int argc, char* argv[])
{
    line* lines  = NULL;

    char* filename = malloc(sizeof(char) * strlen(argv[1]) + 5);
    if (!filename)
    {
        printf("Filename malloc failed\n");
        return 1;
    }
    sprintf(filename, "%s.txt", argv[1]);

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, true);

    getmaxyx(stdscr, max_y, max_x);
    lines = add_line(lines);
    line* current_line = lines;

    FILE* read_file = fopen(filename, "r+");
    if (read_file != NULL)
    {
        char read_buffer;
        while (fread(&read_buffer, 1, 1, read_file) != 0)
        {
            addat_cursor(read_buffer, current_line);
            current_line->number_characters++;
            if (current_line->number_characters == max_x)
            {
                current_line->next_line = add_line(current_line);
                current_line = current_line->next_line;
            }
        }

        for (line* ptr = lines; ptr != NULL; ptr = ptr->next_line)
        {
            for (char* buffer_ptr = ptr->buffer; buffer_ptr < ptr->buffer + max_x; buffer_ptr++)
            {
                printw("%c", *buffer_ptr);
            }
        }
        getyx(stdscr, y, x);
        refresh;
        fclose(read_file);
    }

    int input;
    // Should probably be switch statement
    while ((input = getch()) != KEY_F(1))
    {
        if (input == KEY_F(1))
        {
            break;
        }
        else if (input == KEY_LEFT)
        {
            // If at the start of the line and there is a previous line
            if (current_line->gap_start == current_line->buffer && current_line->previous_line != NULL)
            {
                current_line = current_line->previous_line;
                current_line->gap_start = current_line->buffer + current_line->number_characters - 1;
                current_line->gap_end = current_line->buffer + max_x - 1;
                y--;
                x = max_x;
            }
            // If you're in the middle of the line
            else
            {
                current_line->gap_start--;
                *current_line->gap_end = *current_line->gap_start;
                current_line->gap_end--;     
                x--;
            }
            move(y, x);
            refresh();
        }
        else if (input == KEY_RIGHT)
        {
            // If at the end of the line and the next line already exists
            if (current_line->gap_start == current_line->buffer + max_x - 1 && current_line->next_line != NULL)
            {
                current_line = current_line->next_line;
                current_line->gap_start = current_line->buffer;
                current_line->gap_end = current_line->buffer + max_x - 1;
                y++;
                x = 0;
            }
            // If not at the end of the line
            else
            {
                current_line->gap_end++; 
                *current_line->gap_start = *current_line->gap_end;
                current_line->gap_start++;
                x++;
            }
            move(y, x);
            refresh();
        }
        else if (input == KEY_UP)
        {
            // If there is a previous line and it has enough characters, go to that position
            if (current_line->previous_line != NULL && current_line->previous_line->number_characters >= x)
            {
                current_line->previous_line->gap_start = current_line->previous_line->buffer + (current_line->gap_start - current_line->buffer);
                current_line->previous_line->gap_end = current_line->previous_line->buffer + max_x - 1;
                current_line = current_line->previous_line;
                y--;
                move(y, x);
                refresh();
            }
            // If there aren't enough characters
            else if (current_line->previous_line != NULL && current_line->previous_line->number_characters < x)
            {
                current_line->previous_line->gap_start = current_line->previous_line->buffer + current_line->previous_line->number_characters;
                current_line->previous_line->gap_end = current_line->previous_line->buffer + max_x - 1;
                current_line = current_line->previous_line;
                y--;
                x = current_line->number_characters + 1;
                move(y, x);
                refresh();
            }
        }
        else if (input == 127)
        {
            // Go back to previous line if there is one
            if (current_line->gap_start == current_line->buffer && current_line->previous_line != NULL)
            {
                current_line = current_line->previous_line;
                current_line->gap_start = current_line->buffer + current_line->number_characters;
                current_line->gap_end = current_line->buffer + max_x - 1;
                y--;
                x = max_x;
            }
            else
            {
                delete(current_line);
                current_line->number_characters--;
                if (x > 0)
                {
                    x--;
                }
            }

            clear();
            for (line* ptr = lines; ptr != NULL; ptr = ptr->next_line)
            {
                for (char* ptr2 = ptr->buffer; ptr2 < ptr->buffer + max_x; ptr2++)
                {
                    if (ptr->gap_start == ptr->gap_end)
                    {
                        printw("%c", *ptr2);
                    }
                    else if (ptr2 < ptr->gap_start || ptr2 > ptr->gap_end)
                    {
                        printw("%c", *ptr2);
                    }
                }
            }
            move(y, x);
            refresh();
        }
        else if (input == 10)
        {
            addat_cursor(input, current_line);
            current_line->next_line = add_line(current_line);
            current_line = current_line->next_line;
            y++;
            x = 0;
            move(y, x);
            refresh();
        }
        else
        {
            // If at end of line and there is a next line
            if (current_line->gap_start == current_line->buffer + max_x - 1 && current_line->next_line != NULL)
            {
                memmove(current_line->next_line->gap_end, current_line->buffer + max_x - 1, 1);
                current_line->next_line->gap_end--;
                current_line->next_line->gap_start = current_line->next_line->buffer;
                addat_cursor(input, current_line);
                current_line = current_line->next_line;
                y++;
                x = 0;
            }
            // If not at the end of the line
            else
            {
                addat_cursor(input, current_line);
                if (current_line->number_characters < max_x)
                {
                    current_line->number_characters++;
                    x++;
                }

                // If line is full and there's not a next line yet, make a new line
                if (current_line->gap_start == current_line->buffer + max_x - 1 && current_line->next_line == NULL)
                {
                    current_line->next_line = add_line(current_line);
                    current_line = current_line->next_line;
                    y++;
                    x = 0;
                }
            }

            clear();
            for (line* ptr = lines; ptr != NULL; ptr = ptr->next_line)
            {
                for (char* ptr2 = ptr->buffer; ptr2 < ptr->buffer + max_x; ptr2++)
                {
                    if (ptr->gap_start == ptr->gap_end)
                    {
                        printw("%c", *ptr2);
                    }
                    else if (ptr2 < ptr->gap_start || ptr2 > ptr->gap_end)
                    {
                        printw("%c", *ptr2);
                    }
                }
            }
            move(y, x);
            refresh();
            }
    }
    endwin();

    FILE* write_file = fopen(filename, "w+");
    if (write_file == NULL)
    {
        printf("File open failed\n");
        return 1;
    }

    int i = 0;
    for (line* ptr = lines; ptr != NULL; ptr = ptr->next_line)
    {
        for (char* ptr2 = ptr->buffer; ptr2 < ptr->buffer + max_x; ptr2++)
        {
            if (ptr->gap_start == ptr->gap_end)
            {
                fwrite(ptr2, 1, 1, write_file);
                printf("%c", *ptr2);
            }
            else if (ptr2 < ptr->gap_start || ptr2 > ptr->gap_end)
            {
                fwrite(ptr2, 1, 1, write_file);
                printf("%c", *ptr2);
            }
        }
    printf("\n");
    printf("%i\n", ptr->number_characters);
    }

    
    fclose(write_file);
    free(filename);
    free_lines(lines);
    return 0;
}

void addat_cursor(int input, line* current_line)
{
    line** current_line_reference = &current_line;
    char** gap_start_reference = &current_line->gap_start;

    if (current_line->gap_start == current_line->buffer + max_x - 1)
    {
        *current_line->gap_start = input;
    }
    else
    {
        *current_line->gap_start = input;
        (*gap_start_reference)++;
    }
}

void move_left(line* current_line)
{   
    line** current_line_reference = &current_line;
    char** gap_start_reference = &current_line->gap_start;
    char** gap_end_reference = &current_line->gap_end;
    int* y_reference = &y;
    int* x_reference = &x;

    if (current_line->gap_start == current_line->buffer && current_line->previous_line == NULL)
    {
        return;
    }
    else if (current_line->gap_start == current_line->buffer && current_line->previous_line != NULL)
    {
        current_line->gap_start = current_line->buffer + current_line->number_characters - 1;
        current_line->gap_end = current_line->gap_start;
    }
    else
    {
        (*gap_start_reference)--;
        *current_line->gap_end = *current_line->gap_start;
        (*gap_end_reference)--;  
        *x_reference--;        
    }
}

void move_right(line* current_line)
{
    line** current_line_reference = &current_line;
    char** gap_start_reference = &current_line->gap_start;
    char** gap_end_reference = &current_line->gap_end;

        (*gap_end_reference)++; 
        *current_line->gap_start = *current_line->gap_end;
        (*gap_start_reference)++;
        return;
}

int delete(line* current_line)
{
    char** gap_start_reference = &current_line->gap_start;
    char** gap_end_reference = &current_line->gap_end;
    line** current_line_reference = &current_line;

    if (current_line->gap_start == current_line->buffer && current_line->previous_line == NULL)
    {
        return 1;
    }
    else if (current_line->gap_start == current_line->buffer && current_line->previous_line != NULL)
    {
        current_line->previous_line->gap_start = current_line->buffer + current_line->number_characters - 1;
        current_line->previous_line->gap_end = current_line->previous_line->buffer + max_x - 1;
        *current_line_reference = current_line->previous_line;
        return 2;
    }
    else
    {
        (*gap_start_reference)--;
        return 0;
    }
}

line* add_line(line* previous_line)
{
    line* new_line = malloc(sizeof(line));
    
    new_line->previous_line = previous_line;
    new_line->next_line = NULL;
    new_line->buffer = calloc(max_x, sizeof(char));
    new_line->gap_start = new_line->buffer;
    new_line->gap_end = new_line->buffer + max_x - 1;
    new_line->number_characters = 0;

    return new_line;
}

void free_lines(line* ptr)
{
    if (ptr == NULL)
    {
        return;
    }
    free_lines(ptr->next_line);
    free(ptr->buffer);
    free(ptr);
}

void shuffle(line* line)
{
    if (line->number_characters == max_x && line->next_line != NULL)
    {
        shuffle(line->next_line);
        memmove(line->next_line->buffer, line->buffer + max_x - 1, 1);
        return;
    }

    char* gap_start_reference = line->gap_start;
    memmove(line->buffer + 1, line->buffer, line->number_characters);
    *(gap_start_reference)++;
    return;
}