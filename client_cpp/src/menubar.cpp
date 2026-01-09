#include "../include/menubar.hpp"
#include <iostream>

    MenuBar::MenuBar(WINDOW* _Win, Menu* _Menus, int _Count)
        : count(_Count), selected(-1), window(_Win), menus(_Menus) 
    {
        int x_max, y_max, x_beg, y_beg;
        getmaxyx(window, y_max, x_max);
        getbegyx(window, y_beg, x_beg);
        menu_window = newwin(y_max - 2, x_max - 4, y_beg + 1, x_beg + 2);
        keypad(menu_window, true);
        wrefresh(menu_window);

        int draw_pos = 2;
        for (int i = 0; i < count; i++) {
            this->menus[i].setPos(draw_pos);
            draw_pos += (_Menus[i].getTitle().length() + 2);
        }

        draw();
    }

    void MenuBar::draw() {
        for (int i = 0; i < count; i++) {
            if (selected == i) {
                wattron(window, COLOR_PAIR(2));
                mvwaddstr(window, 0, menus[i].getPos(), menus[i].getTitle().c_str());
                wattroff(window, COLOR_PAIR(2));
                wrefresh(window);

                int input;
                bool exit_loop = false;
                drawMenuWindow(menus[i]);
                wrefresh(menu_window);

                while (!exit_loop && (input = wgetch(menu_window)))
                {
                    switch (input)
                    {
                    case KEY_UP:
                        menus[i].selectPrev();
                        break;
                    case KEY_DOWN:
                        menus[i].selectNext();
                        break;
                    case 10:
                        menus[i].executeMenuItem();
                        exit_loop = true;
                        break;
                    default:
                        exit_loop = true;
                        break;
                    }
                    drawMenuWindow(menus[i]);
                }
                menus[i].setSelected(0);
                werase(menu_window);
                wrefresh(menu_window);
                reset();

            } else {
                mvwaddstr(window, 0, menus[i].getPos(), menus[i].getTitle().c_str());
            }
        }
        selected = -1;

        mvwaddstr(window, 2, 3, "Weather facts for location: ");
    }

    void MenuBar::drawMenuWindow(Menu _Menu) {
        int x_max = getmaxx(menu_window);

        for (int i = 0; i < _Menu.getItemCount(); i++) {
            std::string menu_text = _Menu.getMenuItem(i).getLabel();
            mvwaddstr(menu_window, i, 3, menu_text.c_str());
            if (_Menu.getSelectedItem() == i) {
                //Selected menu item
                mvwchgat(menu_window, i, 0, x_max, A_NORMAL, 2, NULL);
            } else {
                //Non-selected menu item
                mvwchgat(menu_window, i, 0, x_max, A_STANDOUT, 0, NULL);
            }
            wrefresh(menu_window);
        }
    }

    void MenuBar::reset() {
        for (int i = 0; i < count; i++) {
        mvwaddstr(window, 0, menus[i].getPos(), menus[i].getTitle().c_str());
        }
        wrefresh(window);
    }

    void MenuBar::detectInput(char _Key) {
        for (int i = 0; i < count; i++) {
            if ((_Key == menus[i].getKey())) {
                selected = i;
            }
        }
    }

    WINDOW* MenuBar::getWindow() {return this->window;}