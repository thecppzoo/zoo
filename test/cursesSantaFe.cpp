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

    void put(int x, int y, char c) {
        mvwaddch(ptr_, x, y, c);
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
    auto ant = env.ant_;
    auto ad = ant.dir;
    char direction;
    switch(ad.x) {
        case 1: direction = '>'; break;
        case 0: ad.y = -1 ? '^' : 'v'; break;
        case -1: direction = '<'; break;
        default: __builtin_unreachable();
    }
    w.put(1 + ant.pos.y, 1 + ant.pos.x, direction);
    w.refresh();
}

CursesWindow *g_cw = nullptr;
ArtificialAntEnvironment g_aae;

#include <iostream>

int main(int argc, const char *argv[]) {
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    curs_set(0); // Hide cursor

    auto updater = [](auto &e, auto &i, void *r) {
        drawEnvironment(*g_cw, g_aae);
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
    };

    char
        simpleIndividual[] = { Prog2, IFA, Move, TurnRight },
        conversion[sizeof(simpleIndividual) * 3];
    zoo::conversionToWeightedElement<ArtificialAnt>(
        conversion, simpleIndividual
    );
    zoo::WeightedPreorder<ArtificialAnt> indi(conversion);

    // Create a window for the simulation
    int winHeight = 40, winWidth = 80;
    {
        CursesWindow win(winHeight, winWidth, 0, 0);
        g_cw = &win;

        drawEnvironment(win, g_aae);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        do {
            eee(
                g_aae, indi,
                reinterpret_cast<void *>(
                    static_cast<
                        void (*)
                        (
                            ArtificialAntEnvironment &,
                            zoo::WeightedPreorder<ArtificialAnt> &,
                            void *
                        )
                    >(updater)
                )
            );
        } while(!g_aae.atEnd());
    }
    g_cw = nullptr;

    endwin();
    std::cout << "Off" << std::endl;
    return 0;
}
