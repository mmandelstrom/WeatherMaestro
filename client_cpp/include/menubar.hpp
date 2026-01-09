#include <ncurses.h>
#include <string>

#include "menu.hpp"

class MenuBar {
    private:
        int count;
        int selected;
        WINDOW* window;
        WINDOW* menu_window;
        Menu* menus;

        void drawMenuWindow(Menu menu);

    public:
        MenuBar(WINDOW* _Win, Menu* _Menus, int _Count);
        void draw();
        void reset();
        void detectInput(char _Key);
        WINDOW* getWindow();

};