#ifndef __MENUITEM_HPP_
#define __MENUITEM_HPP_

#include <string>
#include <functional>

class MenuItem {
private:
    std::string label;
    std::function<void()> action;
public:
    MenuItem(std::string _Label, std::function<void()> _Action = nullptr);
    std::string getLabel();
    void execute();



};


#endif