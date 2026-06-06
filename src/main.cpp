
#include <iostream>
#include <ncurses.h>

int main(int argc, char* argv[]) {
    initscr();            
    printw("Hello, NCurses World! Press any key to exit..."); 
    refresh();            
    getch();              
    endwin();             
    
    return 0;
}