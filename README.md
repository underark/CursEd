# CursEd (Curse Editor)
### Video demo:
https://www.youtube.com/watch?v=ISDYdrMxwcg
### Description: 
CursEd is a terminal based text editor written in C and using the curses library. It's an extremely minimalistic text editor that (currently) only allows for basic file i/o, adding and deleting text, and basic formatting using paragraphs.

Though CursEd is simple, it has some nice polish to it. For example, making a new paragraph in the middle of a line will transport the text to the right of the cursor to a new paragraph for you. Deleting a paragraph with text in it will also merge it with the previous paragraph, if it exists. CursEd will also remember 'failed' up and down cursor movement attempts and move the cursor there when it is next available on a line.

CursEd uses gap buffers to handle text insertion and deletion. I initially considered using ropes but found this too complicated to understand, so I settled for gap buffers. 

CursEd's first implementation used one large buffer that would grow and shrink. I started running into issues with displaying the document as I was incrementing the y and x values for cursor position manually, which caused desync between the actual buffer position and the on-screen cursor due to how new line characters appeared in the buffer. I didn't like that new line characters were being treated as a 'real' character, despite never being visible to the user, so I abandoned this.

CursEd now uses two structures, lines and paragraphs, to structure the document. Paragraphs are the head of a doubly-linked list of lines. Paragraphs are also themselves a doubly-linked list of paragraphs. Paragraphs track the start and end of the paragraph through pointers. A new paragraph is created when the user presses the enter key.

New line characters are not present in CursEd. They are not saved in a buffer. Instead, they are printed via curses when the end of a paragraph is detected (the final line has a NULL next_line pointer). They are saved in the final document as new line characters, but when the document is re-opened, the document structure (paragraphs and lines) is rebuilt from scratch following the same logic that was used to save them: new line characters mean a new paragraph and a new line is made each time the current one fills up.
 
I chose to do it this way because, as mentioned above, I don't like that new line characters would potentially take up a space in the buffer, despite not being a visible character on the screen. This means that the gap start (with the current software logic) to move over the new line character and overwrite it, which isn't possible in any text editor that I'm aware of. These are the main reasons I opted not to have any new line characters in the line buffers.

Lines contain a buffer that is exactly the size of the terminal window. I decided to go with a buffer of this size because it allows the document to be displayed cleanly. Each line has a line number that is used when printing the document to decide which lines are printed at any given time. If the buffer were any larger or smaller, there would not be an easy way to know which parts of the document are in the viewport, as the text would appear first-in-first-out, so to speak.
One struggle is that line wrapping needs to be handled manually e.g. characters need to actually move between buffers to be displayed correctly. However, one benefit of the line/paragraph structure is that only one paragraph's worth of data ever needs to be considered when line wrapping (as opposed to the whole document) as each paragraph heads its own linked list. I think this makes it slightly more efficient than one large buffer.

One final design decision that I'm proud of is how the cursor position is handled. The window in curses is '0-indexed', so for a window of 80 characters, it spans from position 0-79, just like the buffer array. The cursor position is tied to where the gap start is in the buffer so it will always show up where the cursor position actually is. The y-axis is also calculated based on the line number, so it will always reflect which line is currently active.

I think this is a nice fusion between the functionality of the editor itself and how it's displayed. Rather than incrementing the values manually, it can be passed off to a function that inspects what the document's current state is and displays the cursor based on that.

Thank you for reading!
