#include "../client_cpp/include/menuitem.hpp"

MenuItem::MenuItem(std::string _Label, std::function<void()> _Action) 
    : label(_Label), action(_Action) {}

std::string MenuItem::getLabel() {return this->label;}

void MenuItem::execute() {
    if (action) action();
}