#include "zoo/gp/ArtificialAntEnvironment.h"

#include <ncurses.h>
//#include <vector>
//#include <utility>
#include <thread>
#include <chrono>

struct CursesWindow {
    WINDOW *ptr_;

    CursesWindow(int h, int w, int x1, int x2):
        ptr_(newwin(h, w, x1, x2))
    {}

    ~CursesWindow() { delwin(ptr_); }

    void print(int x, int y, const char *chars) {
        mvwprintw(ptr_, x, y, chars);
    }

    void refresh() { wrefresh(ptr_); }

    operator WINDOW *() { return ptr_; }
};

// Function to draw the environment
void drawEnvironment(
    CursesWindow &w,
    ArtificialAntEnvironment &env
) {
    using AAE = ArtificialAntEnvironment;
    werase(w); // Clear the window
    
    // Offset to position the grid in the center
    auto
        offsetX = 4,
        offsetY = 4;
    
    // Draw the border
    box(w, 0, 0);
    w.print(0, 2, " Santa Fe Trail Simulation ");
    
    // Draw the food elements
    for(auto y = AAE::GridHeight; y--; ) {
        for(auto x = AAE::GridWidth; x--; ) {
            if(! env.food_[y][x]) { continue; }
            w.print(y + 1, 1 + 2*x, "[]");
        }
    }

    w.refresh();
}

int main(int argc, const char *argv[]) {
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    curs_set(0); // Hide cursor

    // Create a window for the simulation
    int winHeight = 40, winWidth = 80;
    {
        CursesWindow win(winHeight, winWidth, 0, 0);
        ArtificialAntEnvironment aae;

        drawEnvironment(win, aae);

        // Animation loop
        while(true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Delay
        }
    }
    endwin();
    return 0;
}
