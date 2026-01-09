#ifndef __MENUITEM_HPP_
#define __MENUITEM_HPP_

#include <string>

class MenuItem {
private:
    std::string label;
    void (*action)();
public:
    MenuItem(std::string _Label, void (*action)());
    std::string getLabel();
    void execute();



};


#endif