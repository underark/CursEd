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
void move_left_one(line* current_line);
void move_right_one(line* current_line);
void move_cursor_to(line* line, int destination);
void delete(line* current_line);
void shuffle_end(paragraph* current_paragraph, line* current_line, int line_counter);
void shuffle_start(paragraph* current_paragraph, line* current_line);
void copy_lines(line* current_line, paragraph* target_paragraph);

// Data structure functions
line* add_line(line* document_start);
void free_lines(line* ptr);
paragraph* add_paragraph(paragraph* current_paragraph);
void free_paragraphs(paragraph* ptr);

//Display functions
void print_lines(paragraph* paragraphs);
void write_paragraphs(paragraph* paragraphs, FILE* write_file);
void update_view(line* current_line);
void update_cursor_position(line* current_line);
void fix_line_numbers(paragraph* current_paragraph);

// These are used to track how many characters a line should be based on terminal size
int max_x = 0;
int max_y = 0;

// These are used to track the coordinates for the cursor
int x = 0;
int y = 0;
int down_fail_value = 0;
int up_fail_value = 0;

// Tracks where the first line of the display should be drawn from
int display_top = 0;
int display_bottom = 0;

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

    // Setting the display value because coordinates are '0 indexed' so the final viewable line is actually max - 1
    display_bottom = max_y - 1;

    paragraph* paragraphs = add_paragraph(NULL);

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

        update_view(current_line);
        print_lines(paragraphs);
        
        getyx(stdscr, y, x);
        // Doing this routine to set the cursor correctly because printw will advance it to the next line
        if (current_line->line_number < max_y - 1)
        {
            y--;
        }
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
                current_paragraph = current_paragraph->previous_paragraph;
                current_line = current_paragraph->paragraph_end;
                move_cursor_to(current_line, current_line->number_characters);
            }
            // If at the start of the line and there is a previous line
            else if (current_line->gap_start == current_line->buffer && current_line->previous_line != NULL)
            {
                current_line = current_line->previous_line;
                int destination = (current_line->number_characters == max_x) ? max_x - 1 : current_line->number_characters;
                move_cursor_to(current_line, destination);
            }
            // If you're anywhere else in the line
            else if (current_line->gap_start != current_line->buffer)
            {
                move_left_one(current_line);
            }
            clear();
            update_view(current_line);
            update_cursor_position(current_line);
            print_lines(paragraphs);
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
                move_cursor_to(current_line, 0);
            }
            else if (current_line->gap_start == current_line->buffer_end && current_line->next_line != NULL)
            {
                current_line = current_line->next_line;
                move_cursor_to(current_line, 0);
            }
            // Stops us running past the end of the line
            else if (current_line->gap_start != current_line->buffer + current_line->number_characters)
            {
                move_right_one(current_line);
            }
            clear();
            update_view(current_line);
            update_cursor_position(current_line);
            print_lines(paragraphs);
            move(y, x);
            refresh();
        }
        else if (input == KEY_UP)
        {
            int destination = current_line->gap_start - current_line->buffer;
            if (current_line->previous_line == NULL && current_paragraph->previous_paragraph != NULL)
            {
                current_paragraph = current_paragraph->previous_paragraph;
                current_line = current_paragraph->paragraph_end; 
                if (up_fail_value > 0)
                {
                    if (current_line->number_characters >= up_fail_value)
                    {
                        move_cursor_to(current_line, up_fail_value);
                        up_fail_value = 0;
                    }
                    else
                    {
                        move_cursor_to(current_line, current_line->number_characters);
                    }
                }
                else if (current_line->number_characters >= destination)
                {
                    move_cursor_to(current_line, destination);
                }
                else
                {
                    up_fail_value = destination;
                    move_cursor_to(current_line, current_line->number_characters);
                }
            }
            else if (current_line->previous_line != NULL)
            {
                current_line = current_line->previous_line;
                move_cursor_to(current_line, destination);
            }
            clear();
            update_view(current_line);
            update_cursor_position(current_line);
            print_lines(paragraphs);
            move(y, x);
            refresh();
        }
        else if (input == KEY_DOWN)
        {
            int destination = current_line->gap_start - current_line->buffer;
            if (current_line->next_line == NULL && current_paragraph->next_paragraph != NULL)
            {
                current_paragraph = current_paragraph->next_paragraph;
                current_line = current_paragraph->paragraph_start;

                if (down_fail_value > 0)
                {
                    if (current_line->number_characters >= down_fail_value)
                    {
                        move_cursor_to(current_line, down_fail_value);
                        down_fail_value = 0;
                    }
                    else
                    {
                        move_cursor_to(current_line, current_line->number_characters);
                    }
                }
                else if (current_line->number_characters >= destination)
                {
                    move_cursor_to(current_line, destination);
                }
                else
                {
                    down_fail_value = destination;
                    move_cursor_to(current_line, current_line->number_characters);
                }
            }
            else if (current_line->next_line != NULL)
            {
                current_line = current_line->next_line;
                destination = (current_line->number_characters >= destination) ? destination : current_line->number_characters;
                move_cursor_to(current_line, destination);
            }
            clear();
            update_view(current_line);
            update_cursor_position(current_line);
            print_lines(paragraphs);
            move(y, x);
            refresh();
        }
        else if (input == 127)
        {
            if (current_line->gap_start == current_line->buffer && current_line->number_characters > 0 && current_line->previous_line == NULL && current_paragraph->previous_paragraph != NULL)
            {
                paragraph* original_current = current_paragraph;

                copy_lines(current_line, current_paragraph->previous_paragraph);

                current_paragraph = current_paragraph->previous_paragraph;
                current_line = current_paragraph->paragraph_end;

                current_paragraph->next_paragraph = original_current->next_paragraph;
                if (current_paragraph->next_paragraph != NULL)
                {
                    current_paragraph->next_paragraph->previous_paragraph = current_paragraph;
                }
                free_lines(original_current->paragraph_start);
                free(original_current);
                fix_line_numbers(current_paragraph);
            }
            else if (current_line->number_characters == 0 && current_line->previous_line == NULL && current_paragraph->previous_paragraph != NULL)
            {
                paragraph* empty_paragraph = current_paragraph;

                current_paragraph = current_paragraph->previous_paragraph;
                current_line = current_paragraph->paragraph_end;

                int destination = (current_line->number_characters == max_x) ? max_x - 1 : current_line->number_characters;
                move_cursor_to(current_line, destination);

                current_paragraph->next_paragraph = empty_paragraph->next_paragraph;
                if (current_paragraph->next_paragraph != NULL)
                {
                    current_paragraph->next_paragraph->previous_paragraph = current_paragraph;
                    fix_line_numbers(current_paragraph);
                }
                free(empty_paragraph->paragraph_start->buffer);
                free(empty_paragraph->paragraph_start);
                free(empty_paragraph);
            }
            else if (current_line->number_characters == 0 && current_line->previous_line != NULL)
            {
                line* empty_line = current_line;

                current_line = current_line->previous_line;

                int destination = (current_line->number_characters == max_x) ? max_x - 1 : current_line->number_characters;
                move_cursor_to(current_line, destination);
                current_line->number_characters--;

                current_line->next_line = empty_line->next_line;
                if (current_line->next_line != NULL)
                {
                    current_line->next_line->previous_line = current_line;
                    fix_line_numbers(current_paragraph);
                }
                free(empty_line->buffer);
                free(empty_line);
            }
            else if (current_line->number_characters == max_x && current_line->next_line->number_characters > 0)
            {
                int move_size = current_line->buffer_end - current_line->gap_start + 1;
                memmove(current_line->gap_start, current_line->gap_start + 1, move_size);
                
                shuffle_start(current_paragraph, current_line);
                current_line->gap_start--;
                current_line->gap_end--;

                if (current_paragraph->paragraph_end->number_characters == 0)
                {
                    current_paragraph->paragraph_end = current_paragraph->paragraph_end->previous_line;
                    free(current_paragraph->paragraph_end->next_line->buffer);
                    free(current_paragraph->paragraph_end->next_line);
                    current_paragraph->paragraph_end->next_line = NULL;
                    fix_line_numbers(current_paragraph);
                }
            }
            else if (current_line->number_characters > 0)
            {
                delete(current_line);
            }
            clear();
            update_view(current_line);
            update_cursor_position(current_line);
            print_lines(paragraphs);
            move(y, x);
            refresh();
        }
        else if (input == 10)
        {
            if (current_line->gap_start == current_line->buffer + current_line->number_characters)
            {
                if (current_paragraph->next_paragraph == NULL)
                {
                    current_paragraph->paragraph_end = current_line;
                    current_paragraph->next_paragraph = add_paragraph(current_paragraph);
                    current_paragraph = current_paragraph->next_paragraph;
                    current_line = current_paragraph->paragraph_start;
                }
                else if (current_paragraph->next_paragraph != NULL)
                {
                    paragraph* original_next = current_paragraph->next_paragraph;

                    current_paragraph->paragraph_end = current_line;
                    current_paragraph->next_paragraph = add_paragraph(current_paragraph);
                    current_paragraph = current_paragraph->next_paragraph;
                    current_paragraph->next_paragraph = original_next;
                    original_next->previous_paragraph = current_paragraph;
                    current_line = current_paragraph->paragraph_start;
                    fix_line_numbers(current_paragraph);
                }
            }
            else
            {
                if (current_paragraph->next_paragraph == NULL)
                {
                    current_paragraph->paragraph_end = current_line;
                    current_paragraph->next_paragraph = add_paragraph(current_paragraph);
                    copy_lines(current_line, current_paragraph->next_paragraph);
                    current_line->number_characters = current_line->gap_start - current_line->buffer;
                    current_line->gap_end = current_line->buffer_end;

                    free_lines(current_line->next_line);
                    current_line->next_line = NULL;

                    current_paragraph = current_paragraph->next_paragraph;
                    current_line = current_paragraph->paragraph_start;

                    fix_line_numbers(current_paragraph);
                }
                else if (current_paragraph->next_paragraph != NULL)
                {
                    paragraph* original_next = current_paragraph->next_paragraph;

                    current_paragraph->paragraph_end = current_line;
                    current_paragraph->next_paragraph = add_paragraph(current_paragraph);
                    copy_lines(current_line, current_paragraph->next_paragraph);
                    current_line->number_characters = current_line->gap_start - current_line->buffer;
                    current_line->gap_end = current_line->buffer_end;

                    free_lines(current_line->next_line);
                    current_line->next_line = NULL;
                    
                    current_paragraph = current_paragraph->next_paragraph;
                    current_paragraph->next_paragraph = original_next;
                    original_next->previous_paragraph = current_paragraph;
                    
                    current_line = current_paragraph->paragraph_start;
                    fix_line_numbers(current_paragraph);
                }
            }
        clear();
        update_view(current_line);
        update_cursor_position(current_line);
        print_lines(paragraphs);
        move(y, x);
        refresh();
        }
        // Buffer insertion
        else if (input >= 0 && input <= 126)
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
                }
                else
                {
                    addat_cursor(input, current_line);
                    current_line->next_line = add_line(current_line);
                    current_paragraph->paragraph_end = current_line->next_line;
                }

                if (current_paragraph->next_paragraph != NULL)
                {
                    fix_line_numbers(current_paragraph->next_paragraph);
                }
            }
            else if (current_line->number_characters == max_x && current_line->next_line != NULL)
            {
                if (current_line->gap_start == current_line->buffer_end)
                {
                    shuffle_end(current_paragraph, current_line, 1);
                    addat_cursor(input, current_line);
                    current_line = current_line->next_line;
                    current_line->gap_start = current_line->buffer;
                }
                else
                {
                    shuffle_end(current_paragraph, current_line, 1);
                    memmove(current_line->gap_start + 1, current_line->gap_start, current_line->buffer_end - current_line->gap_start);
                    addat_cursor(input, current_line);
                }

                if (current_paragraph->next_paragraph != NULL)
                {
                    fix_line_numbers(current_paragraph->next_paragraph);
                }
            }
            else
            {
                addat_cursor(input, current_line);
            }
        clear();
        update_cursor_position(current_line);
        update_view(current_line);
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
    fclose(write_file);
    free(filename);
    free_paragraphs(paragraphs);
    return 0;
}

void addat_cursor(int input, line* current_line)
{
    if (current_line->number_characters < max_x)
    {
        *current_line->gap_start = input;
        current_line->number_characters++;

        if (current_line->gap_start != current_line->buffer_end)
        {
            current_line->gap_start++;
        }        
    }
    else
    {
        *current_line->gap_start = input;

        if (current_line->gap_start != current_line->buffer_end)
        {
            current_line->gap_start++;
        }        
    }
}

void move_left_one(line* current_line)
{
    if (current_line->number_characters == max_x)
    {
        current_line->gap_start--;
        current_line->gap_end--;     
    }
    else
    {
        current_line->gap_start--;
        *current_line->gap_end = *current_line->gap_start;
        current_line->gap_end--;     
    }
}

void move_right_one(line* current_line)
{
    if (current_line->number_characters == max_x)
    {
        current_line->gap_start++;
        current_line->gap_end++;     
    }
    else
    {
        current_line->gap_end++; 
        *current_line->gap_start = *current_line->gap_end;
        current_line->gap_start++;
    }
}

void move_cursor_to(line* line, int destination)
{
    int current_position = line->gap_start - line->buffer;

    if (line->number_characters == max_x)
    {
        line->gap_start = line->buffer + destination;
        line->gap_end = line->buffer + destination;
    }
    else if (current_position < destination)
    {
        int move_size = destination - current_position;
        memmove(line->gap_start, line->gap_end + 1, move_size);
        line->gap_start = line->buffer + destination;
        line->gap_end += move_size;
    }
    else if (current_position > destination)
    {
        int move_size = current_position - destination;
        line->gap_start = line->buffer + destination;
        line->gap_end -= move_size;
        memmove(line->gap_end + 1, line->gap_start, move_size);
    }
    else
    {
        return;
    }
}

void delete(line* current_line)
{
    if (current_line->number_characters == max_x && current_line->gap_start != current_line->buffer)
    {
        current_line->number_characters--;
    }
    else if (current_line->gap_start != current_line->buffer)
    {
        current_line->gap_start--;
        current_line->number_characters--;
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
    new_line->gap_end = new_line->buffer_end;
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

void copy_lines(line* current_line, paragraph* target_paragraph)
{    
    line* target_line = target_paragraph->paragraph_end;
    int destination = (target_line->number_characters == max_x) ? max_x - 1 : target_line->number_characters;
    move_cursor_to(target_line, destination);

    for (line* line_ptr = current_line; line_ptr != NULL; line_ptr = line_ptr->next_line)
    {
        if (line_ptr != current_line)
        {
            move_cursor_to(line_ptr, 0);
        }

        for (char* buffer_ptr = line_ptr->gap_start; buffer_ptr <= line_ptr->buffer_end; buffer_ptr++)
        {
            if (target_line->number_characters == max_x)
            {
                target_line->next_line = add_line(target_line);
                target_line = target_line->next_line;
                target_paragraph->paragraph_end = target_line;
                addat_cursor(*buffer_ptr, target_line);
            }
            else if (buffer_ptr < line_ptr->gap_start || buffer_ptr > line_ptr->gap_end)
            {
                addat_cursor(*buffer_ptr, target_line);
            }
        }
    }
}

void shuffle_start(paragraph* current_paragraph, line* current_line)
{
    for (line* line_ptr = current_line; line_ptr->next_line != NULL; line_ptr = line_ptr->next_line)
    {
        if (line_ptr->next_line->number_characters == max_x)
        {
            memcpy(line_ptr->buffer_end, line_ptr->next_line->buffer, 1);
            memmove(line_ptr->next_line->buffer, line_ptr->next_line->buffer + 1, max_x - 1);
        }
        else
        {
            if (line_ptr->next_line->number_characters == 0)
            {
                return;
            }
            else
            {
                if (line_ptr->next_line->gap_start == line_ptr->next_line->buffer)
                {
                    memcpy(line_ptr->buffer_end, line_ptr->next_line->gap_end + 1, 1);
                    line_ptr->next_line->gap_end++;
                    line_ptr->next_line->number_characters--;
                }
                else
                {
                    int move_size = line_ptr->next_line->gap_start - line_ptr->next_line->buffer + 1;
                    memcpy(line_ptr->buffer_end, line_ptr->next_line->buffer, 1);
                    memmove(line_ptr->next_line->buffer, line_ptr->next_line->buffer + 1, move_size);
                    line_ptr->next_line->gap_start--;
                    line_ptr->next_line->number_characters--;
                }
            }
        }
    }
}

void shuffle_end(paragraph* current_paragraph, line* current_line, int line_counter)
{
    if (current_line->next_line != NULL && current_line->number_characters == max_x)
    {
        char** next_line_gap_end_reference = &current_line->next_line->gap_end;
        shuffle_end(current_paragraph, current_line->next_line, line_counter + 1);

        // On all the previous full lines
        if (current_line->next_line->number_characters < max_x)
        {
            current_line->next_line->gap_end--;
            memcpy(current_line->next_line->gap_end + 1, current_line->buffer_end, 1);
        }
        else
        {
            memcpy(current_line->next_line->buffer, current_line->buffer_end, 1);
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
        current_paragraph->paragraph_end = current_line->next_line;
    }
    return;
}

void print_lines(paragraph* paragraphs)
{
    for (paragraph* para_ptr = paragraphs; para_ptr != NULL; para_ptr = para_ptr->next_paragraph)
    {
        for (line* ptr = para_ptr->paragraph_start; ptr != NULL; ptr = ptr->next_line)
        {
            if (ptr->line_number >= display_top && ptr->line_number <= display_top + max_y - 1)
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
}

void update_view(line* current_line)
{
    int view_size = max_y - 1;

    if (current_line->line_number < display_top)
    {
        display_top = current_line->line_number;
    }
    else if (current_line->line_number > display_bottom)
    {
        display_top = current_line->line_number - view_size;
    }
}

void update_cursor_position(line* current_line)
{

    y = current_line->line_number - display_top;

    x = current_line->gap_start - current_line->buffer;
}

void fix_line_numbers(paragraph* current_paragraph)
{
    for (paragraph* para_ptr = current_paragraph; para_ptr != NULL; para_ptr = para_ptr->next_paragraph)
    {
        for (line* line_ptr = para_ptr->paragraph_start; line_ptr != NULL; line_ptr = line_ptr->next_line)
        {
            line_ptr->line_number = (line_ptr->previous_line == NULL) ? para_ptr->previous_paragraph->paragraph_end->line_number + 1 : line_ptr->previous_line->line_number + 1;
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
            printf("%i\n", ptr->number_characters);
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
        }
    }
}