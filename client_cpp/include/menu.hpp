#ifndef _CUI_HPP_
#define _CUI_HPP_

#include <ncurses.h>
#include <string>

#include "../client_cpp/include/menuitem.hpp"

class Menu {
    private:
        int item_count;
        int selected_item;
        int start_pos;
        char key;

        std::string title;
        MenuItem* menu_items;

    public:
        Menu(std::string _Title, char _Key, MenuItem* _Menu_items, int _Item_count);

        void executeMenuItem();
        void selectNext();
        void selectPrev();
        std::string getTitle();
        void setPos(int _Pos);
        int getPos();
        char getKey();
        MenuItem getMenuItem(int _Index);
        int getItemCount();
        int getSelectedItem();
        void setSelected(int _Index);
        

};





#endif