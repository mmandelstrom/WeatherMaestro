#include "../include/cities.hpp"

Cities::Cities()
    : count(0) {

    }
        
int Cities::add(City _City) {
    this->cities.push_back(_City);
    this->count++;
    return 0;
}
int Cities::remove() {

}

int Cities::getCity(std::string _Name) {
    auto res = std::find_if(this->cities.begin(), this->cities.end(), [&](City& city){return city.getName() == _Name;});
    //TODO: Vafan är auto??? Hur använder vi detta?
    return 0;
}
