#ifndef _CUI_HPP_
#define _CUI_HPP_

#include <ncurses.h>
#include <string>

class Menu {
    private:
        int item_count;
        int selected_item;
        int start_pos;
        char key;

        std::string title;
        std::string* menu_items;

    public:
        Menu(std::string _Title, char _Key, std::string* _Menu_items, int _Item_count);

        void selectNext();
        void selectPrev();
        std::string getTitle();
        void setPos(int _Pos);
        int getPos();
        char getKey();
        std::string getMenuItem(int _Index);
        int getItemCount();
        int getSelectedItem();
        void setSelected(int _Index);

};





#endif