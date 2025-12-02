
#include "weather_client.hpp"

int main() {
  WeatherClient client("Sundbyberg", "Sweden");
  client.getWeather();

  return 0; 
}
