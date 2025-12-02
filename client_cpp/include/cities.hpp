#include <string>
#include <vector>
#include <algorithm>

#include "city.hpp"

class Cities {
    private:
        std::vector<City> cities;
        u_int16_t count;

    public:
        Cities();
        
        int add(City _City);
        int remove();

        int getCity(std::string _Name);


};