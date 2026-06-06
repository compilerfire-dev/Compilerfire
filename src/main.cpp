#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cctype>
#include <ncurses.h>
#include <filesystem>
#include <algorithm>

int main(int argc, char* argv[]) {
    initscr();            
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    
    // Set ESC delay to a very low value (e.g., 25ms) to make the ESC key 
    // responsive immediately without breaking arrow key escape sequences.
    set_escdelay(25);
    
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

    std::vector<std::string> lines = {""};
    std::vector<std::string> all_commands = {"q", "w", "wq", "e", "SaveAs", "Load"};

    int x = 0; int y = 0;
    int scroll_y = 0;
    const int gutter_width = 5; // Width reserved for line numbers (e.g., "  1 ")
    
    // Command Palette and Popup State
    bool in_popup_mode = false;
    std::vector<std::string> popup_items;
    int popup_selected_idx = 0;
    std::string popup_title = "";
    int palette_selected_idx = -1;

    bool in_command_mode = false;
    std::string command_line = "";

    int max_y = 0, max_x = 0;
    getmaxyx(stdscr, max_y, max_x);

    move(y, x);
    refresh();

    int ch;
    bool running = true;

    while (running) {
        ch = getch(); 

        // Update terminal dimensions in case of resize
        getmaxyx(stdscr, max_y, max_x);

        if (in_popup_mode) { // --- POPUP MODE LOGIC ---
            if (ch == 27) { 
                in_popup_mode = false; 
            } else if (ch == KEY_UP) { 
                if (popup_selected_idx > 0) popup_selected_idx--; 
            } else if (ch == KEY_DOWN) { 
                if (popup_selected_idx < (int)popup_items.size() - 1) popup_selected_idx++; 
            } else if (ch == 10 && !popup_items.empty()) {
                std::string selected_path = popup_items[popup_selected_idx];
                if (popup_title == "Load File") {
                    std::ifstream inFile(selected_path);
                    if (inFile.is_open()) {
                        lines.clear();
                        std::string fl;
                        while (std::getline(inFile, fl)) lines.push_back(fl);
                        if (lines.empty()) lines.push_back("");
                        x = 0; y = 0; scroll_y = 0;
                    }
                } else { // SaveAs
                    std::ofstream outFile(selected_path);
                    if (outFile.is_open()) {
                        for (const auto& l : lines) outFile << l << "\n";
                        outFile.close();
                    }
                }
                in_popup_mode = false;
            }
        } else if (in_command_mode) { // --- COMMAND MODE LOGIC ---
            // Filter commands for palette
            std::string current_input = (command_line.length() > 1) ? command_line.substr(1) : "";
            std::vector<std::string> filtered;
            for (const auto& cmd : all_commands) {
                if (cmd.find(current_input) == 0) filtered.push_back(cmd);
            }
            if (palette_selected_idx >= (int)filtered.size()) palette_selected_idx = filtered.size() - 1;

            if (ch == 27) { // ESC to cancel
                in_command_mode = false;
                command_line = ""; palette_selected_idx = -1;
            } else if (ch == KEY_UP) {
                if (palette_selected_idx < (int)filtered.size() - 1) palette_selected_idx++;
            } else if (ch == KEY_DOWN) {
                if (palette_selected_idx > -1) palette_selected_idx--;
            } else if (ch == 10) { // Enter to execute command
                std::string cmd, args;
                if (palette_selected_idx >= 0 && palette_selected_idx < (int)filtered.size()) {
                    cmd = filtered[palette_selected_idx];
                    args = "";
                } else {
                    std::string full_cmd = command_line.substr(1);
                    size_t space_idx = full_cmd.find(' ');
                    cmd = full_cmd.substr(0, space_idx);
                    args = (space_idx == std::string::npos) ? "" : full_cmd.substr(space_idx + 1);
                }

                if (cmd == "q") {
                    running = false;
                } else if (cmd == "w" || cmd == "wq" || cmd == "SaveAs") {
                    if (cmd == "SaveAs" && args.empty()) {
                        in_popup_mode = true; popup_title = "Save As";
                        popup_items.clear();
                        for (const auto& entry : std::filesystem::directory_iterator("."))
                            if (entry.is_regular_file()) popup_items.push_back(entry.path().filename().string());
                    } else {
                        std::string filename = args.empty() ? "output.txt" : args;
                        std::ofstream outFile(filename);
                        if (outFile.is_open()) {
                            for (const auto& line : lines) outFile << line << "\n";
                            outFile.close();
                        }
                        if (cmd == "wq") running = false;
                    }
                } else if (cmd == "e" || cmd == "Load") {
                    if ((cmd == "e" || cmd == "Load") && args.empty()) {
                        in_popup_mode = true; popup_title = "Load File";
                        popup_items.clear();
                        for (const auto& entry : std::filesystem::directory_iterator("."))
                            if (entry.is_regular_file()) popup_items.push_back(entry.path().filename().string());
                    } else {
                        std::ifstream inFile(args);
                        if (inFile.is_open()) {
                            lines.clear();
                            std::string file_line;
                            while (std::getline(inFile, file_line)) lines.push_back(file_line);
                            if (lines.empty()) lines.push_back("");
                            x = 0; y = 0; scroll_y = 0;
                            inFile.close();
                        }
                    }
                }
                in_command_mode = false;
                command_line = ""; palette_selected_idx = -1;
            } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
                if (command_line.length() > 1) command_line.pop_back();
                else { in_command_mode = false; command_line = ""; }
            } else if (isprint(ch)) {
                command_line += (char)ch;
            }
        } else { // --- EDITING MODE LOGIC ---
            // Normal editing logic
            bool cursor_moved = false;
            switch (ch) {
                // -- Navigation --
                case KEY_UP:
                    if (y > 0) y--;
                    x = std::min(x, (int)lines[y].length());
                    break;
                case KEY_DOWN:
                    if (y < (int)lines.size() - 1) y++;
                    x = std::min(x, (int)lines[y].length());
                    break;
                case KEY_LEFT:
                    if (x > 0) x--;
                    else if (y > 0) { // Wrap to end of previous line
                        y--;
                        x = lines[y].length();
                    }
                    break;
                case KEY_RIGHT:
                    if (x < (int)lines[y].length()) x++;
                    else if (y < (int)lines.size() - 1) { // Wrap to start of next line
                        y++;
                        x = 0;
                    }
                    break;
                    
                case 27: // ESC key
                    in_command_mode = true;
                    command_line = ":";
                    break;

                case KEY_MOUSE:
                    MEVENT event;
                    if (getmouse(&event) == OK) {
                        if (event.bstate & BUTTON4_PRESSED) { if (scroll_y > 0) scroll_y--; }
                        else if (event.bstate & 0x00200000) { if (scroll_y < (int)lines.size() - 1) scroll_y++; }
                        else if (event.y == max_y - 1) {
                            in_command_mode = true;
                            command_line = ":";
                        } else if (in_command_mode && !in_popup_mode) {
                            // Palette selection via mouse
                            std::string cur_in = (command_line.length() > 1) ? command_line.substr(1) : "";
                            std::vector<std::string> filt;
                            for (const auto& c : all_commands) if (c.find(cur_in) == 0) filt.push_back(c);
                            // Palette is drawn from max_y - 2 upwards
                            int palette_row_start = (max_y - 2) - ((int)filt.size() - 1);
                            if (event.y >= palette_row_start && event.y <= max_y - 2) {
                                palette_selected_idx = (max_y - 2) - event.y;
                            }
                        } else if (in_popup_mode) {
                            int click_idx = event.y - (max_y / 4 + 2);
                            if (click_idx >= 0 && click_idx < (int)popup_items.size()) popup_selected_idx = click_idx;
                        }
                    }
                    break;

                case 10: // Enter Key
                    {
                        std::string remainder = lines[y].substr(x);
                        lines[y].erase(x);
                        lines.insert(lines.begin() + y + 1, remainder);
                        y++; x = 0;
                    }
                    break;
                    
                case 15: // Ctrl+O
                    {
                        std::ofstream outFile("output.txt");
                        if (outFile.is_open()) {
                            for (const auto& line : lines) outFile << line << "\n";
                            outFile.close();
                        }
                    }
                    break;
                    
                case KEY_BACKSPACE: case 127: case '\b':
                    if (x > 0) { lines[y].erase(x - 1, 1); x--; }
                    else if (y > 0) {
                        x = lines[y - 1].length();
                        lines[y - 1] += lines[y];
                        lines.erase(lines.begin() + y);
                        y--;
                    }
                    break;
                    
                default:
                    if (isprint(ch)) { lines[y].insert(x, 1, (char)ch); x++; }
                    break;
            }
        }

        // Keep the cursor on screen: scroll if the cursor moves out of view
        int editor_height = max_y - 1; // Reserve bottom line for command prompt
        if (y < scroll_y) {
            scroll_y = y;
        } else if (y >= scroll_y + editor_height) {
            scroll_y = y - editor_height + 1;
        }
        
        erase(); // Clear the screen
        for (int i = 0; i < editor_height && (i + scroll_y) < (int)lines.size(); ++i) {
            int current_line_idx = i + scroll_y;
            // 1. Print the line number in the gutter (right-aligned, 1-based index)
            attron(A_DIM); // Make line numbers slightly dimmer than text
            mvprintw(i, 0, "%3d  ", current_line_idx + 1);
            attroff(A_DIM);

            // 2. Print the text content starting after the gutter
            mvaddstr(i, gutter_width, lines[current_line_idx].substr(0, std::max(0, max_x - gutter_width)).c_str());
        }
        
        if (in_command_mode) {
            // --- RENDER COMMAND PALETTE ---
            std::string cur_in = (command_line.length() > 1) ? command_line.substr(1) : "";
            std::vector<std::string> filt;
            for (const auto& c : all_commands) if (c.find(cur_in) == 0) filt.push_back(c);
            
            for (int i = 0; i < (int)filt.size(); ++i) {
                if (i == palette_selected_idx) attron(A_REVERSE);
                mvprintw(max_y - 2 - i, 0, " %-12s ", filt[i].c_str());
                attroff(A_REVERSE);
            }

            // Draw command line at the bottom
            mvaddstr(max_y - 1, 0, command_line.c_str());
            move(max_y - 1, command_line.length());
        } else if (in_popup_mode) {
            // --- RENDER POPUP ---
            int pw = 40, ph = 12;
            int py = max_y / 4, px = (max_x - pw) / 2;
            attron(A_REVERSE);
            for(int i = 0; i < ph; i++) mvhline(py + i, px, ' ', pw);
            mvprintw(py, px + (pw - popup_title.length()) / 2, popup_title.c_str());
            for(int i = 0; i < (int)popup_items.size() && i < ph - 3; i++) {
                if (i == popup_selected_idx) attroff(A_REVERSE);
                mvprintw(py + 2 + i, px + 2, " %s ", popup_items[i].c_str());
                if (i == popup_selected_idx) attron(A_REVERSE);
            }
            attroff(A_REVERSE);
        } else {
            // Move physical cursor relative to scroll position
            move(y - scroll_y, x + gutter_width);
        }
        refresh(); 
    }

    endwin();             
    
    return 0;
}