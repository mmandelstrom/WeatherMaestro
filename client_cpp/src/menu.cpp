#include "../include/menu.hpp"

    Menu::Menu(std::string _Title, char _Key, MenuItem* _Menu_items, int _Item_count)
        : item_count(_Item_count), selected_item(0), key(_Key), title(_Title), menu_items(_Menu_items)
    {
    }

    void Menu::executeMenuItem() {
        menu_items[selected_item].execute();
    }

    void Menu::selectNext() {
        selected_item++;
        if (selected_item >= item_count) {selected_item = 0;}
    }

    void Menu::selectPrev() {
        selected_item--;
        if (selected_item >= item_count) {selected_item = item_count - 1;}
    }

    std::string Menu::getTitle() {return this->title;}

    void Menu::setPos(int _Pos) {this->start_pos = _Pos;};

    int Menu::getPos() {return this->start_pos;}

    char Menu::getKey() {return this->key;}

    // Returns empty MenuItem upon failure.
    MenuItem Menu::getMenuItem(int _Index) {
        if (_Index >= 0) {
            return this->menu_items[_Index];
        } else {
            return MenuItem(0, nullptr);
        }
    }

    int Menu::getItemCount() {return this->item_count;}

    int Menu::getSelectedItem() {return this->selected_item;}

    void Menu::setSelected(int _Index) {
        if (_Index >= 0 && _Index <= (item_count - 1)) {
            this->selected_item = _Index;
        }
        
    }