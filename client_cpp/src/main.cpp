
#include "../include/weather_client.hpp"
#include "../include/cities.hpp"
#include "../include/city.hpp"

int main() {
  WeatherClient client("Sundbyberg", "Sweden");
  Cities c;
  City c1("Stockholm", "Stockholms Län", "Sverige", 52.123f, 12.234f);
  City c2("Sundbyberg", "Stockholms Län", "Sverige", 52.123f, 12.234f);
  City c3("Malmö", "Skåne Län", "Sverige", 13.456f, 54.234f);

  c.add(c1);
  c.add(c2);
  c.add(c3);

  c.listCities();
  c.getCity("Sune");
  c.getCity("Sundbyberg");
  
  return 0; 
}
