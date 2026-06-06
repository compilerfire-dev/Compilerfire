
#include <iostream>
#include <ncurses.h>

int main(int argc, char* argv[]) {
    initscr();            
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int x = 0; int y = 0;

    int max_y = 0, max_x = 0;
    getmaxyx(stdscr, max_y, max_x);

    move(y, x);
    refresh();

    int ch;
    bool running = true;

    while (running) {
        ch = getch(); 

        switch (ch) {
            // -- Navigation --
            case KEY_UP:
                if (y > 0) y--;
                break;
            case KEY_DOWN:
                if (y < max_y - 1) y++;
                break;
            case KEY_LEFT:
                if (x > 0) x--;
                break;
            case KEY_RIGHT:
                if (x < max_x - 1) x++;
                break;
                
            case 27: // ESC key
                running = false;
                break;
                
            // -- Deletion --
            case KEY_BACKSPACE:
            case 127: 
            case '\b':
                if (x > 0) {
                    x--;
                    mvaddch(y, x, ' '); 
                }
                break;
                
            // -- Typing --
            default:
                if (isprint(ch)) {
                    mvaddch(y, x, ch); 
                    if (x < max_x - 1) x++;
                }
                break;
        }

        move(y, x); 
        
        refresh(); 
    }

    endwin();             
    
    return 0;
}