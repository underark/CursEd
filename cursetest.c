#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct line line;
struct line
{
    line* next_line;
    line* previous_line;
    char* buffer;
    char* buffer_end;
    char* gap_start;
    char* gap_end;
    int number_characters;
    int line_number;
};

typedef struct paragraph paragraph;
struct paragraph
{
    line* paragraph_start;
    line* paragraph_end;
    paragraph* previous_paragraph;
    paragraph* next_paragraph;
};

// Functions for altering the buffer
void addat_cursor(int input, line* current_line);
void move_right(line* current_line);
void move_left(line* current_line);
int delete(line* current_line);
void shuffle_end(line* current_line, int line_counter);

// Data structure functions
line* add_line(line* document_start);
void free_lines(line* ptr);
paragraph* add_paragraph(paragraph* current_paragraph);
void free_paragraphs(paragraph* ptr);

//Display functions
void print_lines(paragraph* paragraphs);
void write_paragraphs(paragraph* paragraphs, FILE* write_file);

// These are used to track how many characters a line should be based on terminal size
int max_x = 0;
int max_y = 0;

// These are used to track the coordinates for the cursor
int x = 0;
int y = 0;

int main(int argc, char* argv[])
{

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

    paragraph* paragraphs = add_paragraph(NULL);

    // These will be used throughout the document to track the current position
    line* current_line = paragraphs->paragraph_start;
    paragraph* current_paragraph = paragraphs;

    FILE* read_file = fopen(filename, "r+");
    if (read_file != NULL)
    {
        char read_buffer;
        // Correctly building the data structure from existing data
        while (fread(&read_buffer, 1, 1, read_file) != 0)
        {
            // Avoiding any new line characters making their way into the buffer as new lines are purely visual in Cursed
            if (current_line->number_characters == max_x - 1)
            {
                current_line->next_line = add_line(current_line);
                current_line->next_line->previous_line = current_line;
                current_line = current_line->next_line;

                addat_cursor(read_buffer, current_line->previous_line);
            }
            else if (read_buffer != '\n')
            {
                addat_cursor(read_buffer, current_line);
            }
            // Preserving the structure of each 'line' in the original file
            else if (read_buffer == '\n')
            {
                current_paragraph->next_paragraph = add_paragraph(current_paragraph);
                current_paragraph->paragraph_end = current_line;
                current_paragraph = current_paragraph->next_paragraph;
                current_line = current_paragraph->paragraph_start;
            }
        }

        for (paragraph* para_ptr = paragraphs; para_ptr != NULL; para_ptr = para_ptr->next_paragraph)
        {
            for (line* ptr = para_ptr->paragraph_start; ptr != NULL; ptr = ptr->next_line)
            {
                for (char* buffer_ptr = ptr->buffer; buffer_ptr < ptr->buffer + max_x; buffer_ptr++)
                {
                    printw("%c", *buffer_ptr);
                }
    
                if (ptr->next_line == NULL)
                {
                    printw("\n");
                }
            }
        }
        // Doing this routine to set the cursor correctly because printw will advance it to the next line
        getyx(stdscr, y, x);
        y--;
        x = current_line->number_characters;
        move(y, x);

        refresh();
        fclose(read_file);
    }

    int input;
    while ((input = getch()) != KEY_F(1))
    {
        if (input == KEY_F(1))
        {
            break;
        }
        else if (input == KEY_LEFT)
        {
            if (current_line->gap_start == current_line->buffer && current_line->previous_line == NULL && current_paragraph->previous_paragraph != NULL)
            {
                line* previous_paragraph_end = current_paragraph->previous_paragraph->paragraph_end;
                memmove(previous_paragraph_end->gap_start, previous_paragraph_end->gap_end + 1, previous_paragraph_end->buffer + max_x - 1 - previous_paragraph_end->buffer);
                previous_paragraph_end->gap_start += previous_paragraph_end->buffer + max_x - 1 - previous_paragraph_end->gap_end;
                previous_paragraph_end->gap_end = previous_paragraph_end->buffer + max_x - 1;
                current_paragraph = current_paragraph->previous_paragraph;
                current_line = previous_paragraph_end;
                y--;
                x = current_line->number_characters;
            }
            // If at the start of the line and there is a previous line
            else if (current_line->gap_start == current_line->buffer && current_line->previous_line != NULL)
            {
                current_line = current_line->previous_line;
                memmove(current_line->gap_start, current_line->gap_end + 1, current_line->buffer + max_x - current_line->gap_end - 1);
                current_line->gap_end = current_line->buffer + max_x - 1;
                current_line->gap_start += current_line->buffer + max_x - current_line->gap_end - 1;
                y--;
                x = current_line->number_characters - 1;
            }
            // If you're anywhere else in the line
            else if (current_line->gap_start != current_line->buffer && current_line->number_characters != max_x)
            {
                current_line->gap_start--;
                *current_line->gap_end = *current_line->gap_start;
                current_line->gap_end--;     
                x--;
            }
            else if (current_line->number_characters == max_x && current_line->gap_start != current_line->buffer)
            {
                current_line->gap_start--;
                current_line->gap_end--;     
                x--;
            }
            move(y, x);
            refresh();
        }
        else if (input == KEY_RIGHT)
        {
            // If at the end of the line and the next line already exists
            // TO DO
            if (current_line->gap_start == current_line->buffer + current_line->number_characters && current_line->next_line == NULL && current_paragraph->next_paragraph != NULL)
            {
                current_paragraph = current_paragraph->next_paragraph;
                current_line = current_paragraph->paragraph_start;
                current_line->gap_end -= current_line->gap_start - current_line->buffer;
                memmove(current_line->gap_end + 1, current_line->buffer, current_line->gap_start - current_line->buffer);
                current_line->gap_start = current_line->buffer;
                y++;
                x = 0;
            }
            else if (current_line->gap_start == current_line->buffer_end && current_line->next_line != NULL)
            {
                current_line = current_line->next_line;
                if (current_line->gap_start != current_line->buffer)
                {
                    current_line->gap_end -= current_line->gap_start - current_line->buffer;
                    memmove(current_line->gap_end + 1, current_line->buffer, current_line->gap_start - current_line->buffer);
                    current_line->gap_start = current_line->buffer;
                }
                y++;
                x = 0;
            }
            // Stops us running past the end of the line
            else if (current_line->gap_start != current_line->buffer + current_line->number_characters && current_line->number_characters != max_x)
            {
                current_line->gap_end++; 
                *current_line->gap_start = *current_line->gap_end;
                current_line->gap_start++;
                x++;
            }
            else if (current_line->number_characters == max_x && current_line->gap_start != current_line->buffer_end)
            {
                current_line->gap_end++; 
                current_line->gap_start++;
                x++;
            }
            move(y, x);
            refresh();
        }
        else if (input == KEY_UP)
        {
            // Moving up a paragraph
            if (current_line->previous_line == NULL && current_paragraph->previous_paragraph != NULL)
            {
                current_paragraph = current_paragraph->previous_paragraph;
                current_line = current_paragraph->paragraph_end; 
                if (current_line->number_characters >= x)
                {
                    if (current_line->number_characters == max_x)
                    {
                        current_line->gap_start = current_line->buffer + x;
                        current_line->gap_end = current_line->gap_start;
                        y--;
                        move(y, x);
                        refresh();
                    }
                    else
                    {
                        // If the cursor position is 'more left' than the gap position
                        if (x < current_line->gap_start - current_line->buffer)
                        {
                            current_line->gap_end -= current_line->gap_start - current_line->buffer - x;
                            memmove(current_line->gap_end + 1, current_line->buffer + x, current_line->gap_start - current_line->buffer - x);
                            current_line->gap_start = current_line->buffer + x;
                        }
                        // If the cursor position is 'more right' than the gap position
                        else if (x > current_line->gap_start - current_line->buffer)
                        {
                            memmove(current_line->gap_start, current_line->gap_end + 1, x - (current_line->gap_start - current_line->buffer));
                            current_line->gap_start += x - (current_line->gap_start - current_line->buffer);
                            current_line->gap_end += x - (current_line->gap_start - current_line->buffer);
                        }
                        y--;
                        move(y, x);
                        refresh();
                    }
                }
                else if (current_line->number_characters < x)
                {
                    memmove(current_line->gap_start, current_line->gap_end + 1, current_line->buffer + max_x - current_line->gap_end - 1);
                    current_line->gap_start += current_line->buffer + max_x - current_line->gap_end - 1;
                    current_line->gap_end = current_line->buffer + max_x - 1;
                    y--;
                    x = current_line->gap_start - current_line->buffer;
                    move(y, x);
                    refresh();
                }    
            }
            // Moving up a line in a paragraph
            // If there is a previous line in the paragraph and it has enough characters, go to that position
            else if (current_line->previous_line != NULL && current_line->previous_line->number_characters >= x)
            {
                // If the line is full, then there's no room to move any existing characters
                // so the gap start and end become the same and we shuffle characters between buffers on insert
                if (current_line->previous_line->number_characters == max_x)
                {
                    current_line->previous_line->gap_start = current_line->previous_line->buffer + x;
                    current_line->previous_line->gap_end = current_line->previous_line->gap_start;
                    current_line = current_line->previous_line;
                    y--;
                    move(y, x);
                    refresh();
                }
                // If there is room then we need to move some stuff to the left or right to maintain the gap correctly
                else
                {
                    current_line = current_line->previous_line;
                    // If the cursor position is 'more left' than the gap position
                    if (x < current_line->gap_start - current_line->buffer)
                    {
                        current_line->gap_end -= current_line->gap_start - current_line->buffer - x;
                        memmove(current_line->gap_end + 1, current_line->buffer + x, current_line->gap_start - current_line->buffer - x);
                        current_line->gap_start = current_line->buffer + x;
                    }
                    // If the cursor position is 'more right' than the gap position
                    if (x > current_line->gap_start - current_line->buffer)
                    {
                        memmove(current_line->gap_start, current_line->gap_end + 1, x - (current_line->gap_start - current_line->buffer));
                        current_line->gap_start += x - (current_line->gap_start - current_line->buffer);
                        current_line->gap_end += x - (current_line->gap_start - current_line->buffer);
                    }
                    y--;
                    move(y, x);
                    refresh();
                }
            }
            // If there aren't enough characters
            else if (current_line->previous_line != NULL && current_line->previous_line->number_characters < x)
            {
                current_line = current_line->previous_line;
                memmove(current_line->gap_start, current_line->gap_end + 1, current_line->buffer + max_x - current_line->gap_end - 1);
                current_line->gap_start += current_line->buffer + max_x - current_line->gap_end - 1;
                current_line->gap_end = current_line->buffer + max_x - 1;
                y--;
                x = current_line->gap_start - current_line->buffer;
                move(y, x);
                refresh();
            }
        }
        else if (input == KEY_DOWN)
        {
            // Moving up a paragraph
            if (current_line->next_line == NULL && current_paragraph->next_paragraph != NULL)
            {
                current_paragraph = current_paragraph->next_paragraph;
                current_line = current_paragraph->paragraph_start;
                if (current_line->number_characters >= x)
                {
                    if (current_line->number_characters == max_x)
                    { 
                        current_line->gap_start = current_line->buffer + x;
                        current_line->gap_end = current_line->gap_start;
                        y++;
                        move(y, x);
                        refresh();
                    }
                    else
                    {
                        // If the cursor position is 'more left' than the gap position
                        if (x < current_line->gap_start - current_line->buffer)
                        {
                            current_line->gap_end -= current_line->gap_start - current_line->buffer - x;
                            memmove(current_line->gap_end + 1, current_line->buffer + x, current_line->gap_start - current_line->buffer - x);
                            current_line->gap_start = current_line->buffer + x;
                        }
                        // If the cursor position is 'more right' than the gap position
                        else if (x > current_line->gap_start - current_line->buffer)
                        {
                            memmove(current_line->gap_start, current_line->gap_end + 1, x - (current_line->gap_start - current_line->buffer));
                            current_line->gap_start += x - (current_line->gap_start - current_line->buffer);
                            current_line->gap_end += x - (current_line->gap_start - current_line->buffer);
                        }
                        y++;
                        move(y, x);
                        refresh();
                    }
                }
                else if (current_line->number_characters < x)
                {
                    memmove(current_line->gap_start, current_line->gap_end + 1, current_line->buffer + max_x - current_line->gap_end - 1);
                    current_line->gap_start += current_line->buffer + max_x - current_line->gap_end - 1;
                    current_line->gap_end = current_line->buffer + max_x - 1;
                    y++;
                    x = current_line->gap_start - current_line->buffer;
                    move(y, x);
                    refresh();
                }    
            }
            // If there's a next line and it has enough characters
            else if (current_line->next_line != NULL && current_line->next_line->number_characters >= x)
            {
                // Full line is the same as before...
                if (current_line->next_line->number_characters == max_x)
                {
                    current_line->next_line->gap_start = current_line->next_line->buffer + x;
                    current_line->next_line->gap_end = current_line->next_line->gap_start;
                    current_line = current_line->next_line;
                    y++;
                }
                // Need to move things around again depending on the current position of the buffer
                else
                {
                    current_line = current_line->next_line;
                    if (x < current_line->gap_start - current_line->buffer)
                    {
                        current_line->gap_end -= current_line->gap_start - current_line->buffer - x;
                        memmove(current_line->gap_end + 1, current_line->buffer + x, current_line->gap_start - current_line->buffer - x);
                        current_line->gap_start = current_line->buffer + x;

                    }
                    else if (x > current_line->gap_start - current_line->buffer)
                    {
                        memmove(current_line->gap_start, current_line->gap_end + 1, x - (current_line->gap_start - current_line->buffer));
                        current_line->gap_start += x - (current_line->gap_start - current_line->buffer);
                        current_line->gap_end += x - (current_line->gap_start - current_line->buffer);
                    }
                    y++;
                }
            }
            else if (current_line->next_line != NULL && current_line->next_line->number_characters < x)
            {
                current_line = current_line->next_line;
                memmove(current_line->gap_start, current_line->gap_end + 1, current_line->buffer + max_x - current_line->gap_end - 1);
                current_line->gap_start += current_line->buffer + max_x - current_line->gap_end - 1;
                current_line->gap_end = current_line->buffer + max_x - 1;
                y++;
                x = current_line->gap_start - current_line->buffer;
            }
            move(y, x);
            refresh();
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
            print_lines(paragraphs);
            move(y, x);
            refresh();
        }
        else if (input == KEY_F(2))
        {
            // Making a new paragraph mid line
            if (current_line->gap_start != current_line->buffer + current_line->number_characters)
            {
                // Set up the next paragraph and first line
                if (current_paragraph->next_paragraph == NULL)
                {
                    current_paragraph->next_paragraph = add_paragraph(current_paragraph);
                    current_paragraph->paragraph_end = current_line;
                    current_paragraph = current_paragraph->next_paragraph;
    
                    // Update character counts
                    current_line->number_characters -= current_line->buffer + max_x - 1 - current_line->gap_end;
                    current_paragraph->paragraph_start->number_characters += current_line->buffer + max_x - 1 - current_line->gap_end;
    
                    // Move the data in the buffer over, adjust the gap
                    memmove(current_paragraph->paragraph_start->gap_end + 1, current_line->gap_end + 1, current_line->buffer + max_x - 1 - current_line->gap_end);
                    current_line->gap_end += current_line->buffer + max_x - 1 - current_line->gap_end;
    
                    current_line = current_paragraph->paragraph_start;
                    current_line->line_number = current_paragraph->previous_paragraph->paragraph_end->line_number + 1;
                }
                else
                {
                    paragraph* original_next_paragraph = current_paragraph->next_paragraph;

                    current_paragraph->next_paragraph = add_paragraph(current_paragraph);
                    original_next_paragraph->previous_paragraph = current_paragraph->next_paragraph;

                    current_paragraph->paragraph_end = current_line;
                    current_paragraph = current_paragraph->next_paragraph;
                    current_paragraph->next_paragraph = original_next_paragraph;
                    current_paragraph->paragraph_start->gap_end -= current_line->buffer + max_x - 1 - current_line->gap_end;
    
                    // Update character counts
                    current_line->number_characters -= current_line->buffer + max_x - 1 - current_line->gap_end;
                    current_paragraph->paragraph_start->number_characters += current_line->buffer + max_x - 1 - current_line->gap_end;
    
                    // Move the data in the buffer over, adjust the gap
                    memmove(current_paragraph->paragraph_start->gap_end + 1, current_line->gap_end + 1, current_line->buffer + max_x - 1 - current_line->gap_end);
                    current_line->gap_end += current_line->buffer + max_x - 1 - current_line->gap_end;
    
                    current_line = current_paragraph->paragraph_start;
                    current_line->line_number = current_paragraph->previous_paragraph->paragraph_end->line_number + 1;
                }
            }
            // Making a new paragraph elsewhere
            else
            {
                if (current_paragraph->next_paragraph != NULL)
                {
                    paragraph* original_next_paragraph = current_paragraph->next_paragraph;

                    current_paragraph->next_paragraph = add_paragraph(current_paragraph);
                    original_next_paragraph->previous_paragraph = current_paragraph->next_paragraph;
                    current_paragraph->next_paragraph->next_paragraph = original_next_paragraph;

                    current_paragraph->paragraph_end = current_line;
                    current_paragraph = current_paragraph->next_paragraph;
                    current_line = current_paragraph->paragraph_start;     
                    current_line->line_number = current_paragraph->previous_paragraph->paragraph_end->line_number + 1;               
                }
                else
                {
                    current_paragraph->next_paragraph = add_paragraph(current_paragraph);
                    current_paragraph->paragraph_end = current_line;
                    current_paragraph = current_paragraph->next_paragraph;
                    current_line = current_paragraph->paragraph_start;
                    current_line->line_number = current_paragraph->previous_paragraph->paragraph_end->line_number + 1;
                }
            }
            y++;
            x = 0;
            clear();
            print_lines(paragraphs);
            move(y, x);
            refresh();
        }
        // Buffer insertion
        else
        {
            // If line is full and there's not a next line yet, make a new line
            if (current_line->number_characters == max_x - 1 && current_line->next_line == NULL)
            {
                if (current_line->gap_start == current_line->buffer_end)
                {
                    addat_cursor(input, current_line);
                    current_line->next_line = add_line(current_line);
                    current_line = current_line->next_line;
                    current_paragraph->paragraph_end = current_line;
                    y++;
                    x = 0;
                    *current_line->gap_end = *current_line->gap_start;
                }
                else
                {
                    addat_cursor(input, current_line);
                    current_line->next_line = add_line(current_line);
                    current_paragraph->paragraph_end = current_line->next_line;
                }
            }
            // If adding at the end of a full line
            else if (current_line->number_characters == max_x && current_line->next_line != NULL)
            {
                if (current_line->gap_start == current_line->buffer_end)
                {
                    shuffle_end(current_line, 1);
                    addat_cursor(input, current_line);
                    current_line = current_line->next_line;
                    current_line->gap_start = current_line->buffer;
                    y++;
                    x = 0;
                }
                else
                {
                    shuffle_end(current_line, 1);
                    memmove(current_line->gap_start + 1, current_line->gap_start, current_line->buffer_end - current_line->gap_start);
                    addat_cursor(input, current_line);
                    x++;
                }
            }
            // If adding to anywhere other than the end of a full line
            // If inputting mid line
            else
            {
                addat_cursor(input, current_line);
                x++;
            }

        // Handles the display of the document
        clear();
        print_lines(paragraphs);
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

    write_paragraphs(paragraphs, write_file);
    
    printf("%i\n", current_line->line_number);
    fclose(write_file);
    free(filename);
    free_paragraphs(paragraphs);
    return 0;
}

void addat_cursor(int input, line* current_line)
{
    line** current_line_reference = &current_line;
    char** gap_start_reference = &current_line->gap_start;

    if (current_line->number_characters < max_x)
    {
        *current_line->gap_start = input;
        current_line->gap_start++;
        current_line->number_characters++;
    }
    else
    {
        *current_line->gap_start = input;
        current_line->gap_start++;
    }
}

void move_left(line* current_line)
{   
}

void move_right(line* current_line)
{
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
    new_line->buffer_end = new_line->buffer + max_x - 1;
    new_line->gap_start = new_line->buffer;
    new_line->gap_end = new_line->buffer + max_x - 1;
    new_line->number_characters = 0;

    if (previous_line != NULL)
    {
        new_line->line_number = previous_line->line_number + 1;
    }
    else
    {
        new_line->line_number = 0;
    }

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

paragraph* add_paragraph(paragraph* previous_paragraph)
{
    paragraph* new_paragraph = malloc(sizeof(paragraph));

    new_paragraph->previous_paragraph = previous_paragraph;
    new_paragraph->next_paragraph = NULL;
    new_paragraph->paragraph_start = add_line(NULL);
    new_paragraph->paragraph_end = new_paragraph->paragraph_start;

    if (previous_paragraph != NULL)
    {
        new_paragraph->paragraph_start->line_number = previous_paragraph->paragraph_end->line_number + 1;
    }

    return new_paragraph;
}

void free_paragraphs(paragraph* ptr)
{
    if (ptr == NULL)
    {
        return;
    }
    free_paragraphs(ptr->next_paragraph);
    free_lines(ptr->paragraph_start);
    free(ptr);
}

void shuffle_end(line* current_line, int line_counter)
{
    // We need these variables to change the values within the function because it's pass by value
    char** gap_end_reference = &current_line->gap_end;
    char** gap_start_reference = &current_line->gap_start;

    if (current_line->next_line != NULL && current_line->number_characters == max_x)
    {
        char** next_line_gap_end_reference = &current_line->next_line->gap_end;
        shuffle_end(current_line->next_line, line_counter + 1);

        // On all the previous full lines
        if (current_line->next_line->number_characters < max_x)
        {
            current_line->next_line->gap_end--;
            memmove(current_line->next_line->gap_end + 1, current_line->buffer_end, 1);
        }
        else
        {
            memmove(current_line->next_line->buffer, current_line->buffer_end, 1);
        }

        // Get the hell out on the first line
        if (line_counter == 1)
        {
            return;
        }
        // Jiggle stuff around on subsequent lines
        else
        {
            memmove(current_line->buffer + 1, current_line->buffer, max_x - 1);
            return;    
        }
    }
    // On the empty line
    else if (current_line->number_characters < max_x)
    {
        current_line->gap_end -= current_line->gap_start - current_line->buffer;
        memmove(current_line->gap_end + 1, current_line->buffer, current_line->gap_start - current_line->buffer);
        current_line->number_characters++;
        if (current_line->gap_start != current_line->buffer)
        {
            current_line->gap_start = current_line->buffer;    
        }
    }
    
    if (current_line->number_characters == max_x)
    {
        current_line->next_line = add_line(current_line);
    }
    return;
}

void print_lines(paragraph* paragraphs)
{
    for (paragraph* para_ptr = paragraphs; para_ptr != NULL; para_ptr = para_ptr->next_paragraph)
    {
        for (line* ptr = para_ptr->paragraph_start; ptr != NULL; ptr = ptr->next_line)
        {
            for (char* ptr2 = ptr->buffer; ptr2 < ptr->buffer + max_x; ptr2++)
            {
                if (ptr->number_characters == max_x)
                {
                    printw("%c", *ptr2);
                }
                else if (ptr2 < ptr->gap_start || ptr2 > ptr->gap_end)
                {
                    printw("%c", *ptr2);
                }            
            }
            if (ptr->next_line == NULL)
            {
                printw("\n");
            }
        }
    }
}

void write_paragraphs(paragraph* paragraphs, FILE* write_file)
{
    char enter = '\n';
    for (paragraph* para_ptr = paragraphs; para_ptr != NULL; para_ptr = para_ptr->next_paragraph)
    {
        for (line* ptr = para_ptr->paragraph_start; ptr != NULL; ptr = ptr->next_line)
        {
            for (char* ptr2 = ptr->buffer; ptr2 < ptr->buffer + max_x; ptr2++)
            {
                if (ptr->number_characters == max_x)
                {
                    fwrite(ptr2, 1, 1, write_file);
                    //printf("%c", *ptr2);
                }
                else
                {
                    if (ptr2 < ptr->gap_start || ptr2 > ptr->gap_end)
                    {
                        fwrite(ptr2, 1, 1, write_file);
                    }
                    //printf("%c", *ptr2);
                }
            }
            if (ptr->next_line == NULL && para_ptr->next_paragraph != NULL)
            {
                fwrite(&enter, 1, 1, write_file);
            }
            printf("%i\n", ptr->number_characters);
        }
    }
}