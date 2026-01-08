
#include "../include/weather_client.hpp"
#include "../include/cities.hpp"
#include "../include/city.hpp"

#include <ncurses.h>

void resetMenu(WINDOW* _Win) {
  mvwprintw(_Win, 0, 2, "[City]");
  mvwprintw(_Win, 0, 10, "[Weather]");
  mvwprintw(_Win, 0, 21, "[Settings]");
}

int main() {

  
  initscr();
  noecho();

  if (!has_colors()) {
    endwin();
    std::cout << "Your terminal does not support colors!" << std::endl;
    return 1;
  }
  start_color();
  init_pair(1, COLOR_WHITE, COLOR_BLUE);
  init_pair(2, COLOR_WHITE, COLOR_RED);
  bkgd(COLOR_PAIR(1));

  curs_set(0);
  int term_y, term_x;
  getmaxyx(stdscr, term_y, term_x);
  WINDOW* main = newwin(term_y/1.5, term_x - 4, 1, 2);
  box(main, 0, 0);

  mvwprintw(main, 0, 2, "[City]");
  mvwprintw(main, 0, 10, "[Weather]");
  mvwprintw(main, 0, 21, "[Settings]");

  if (term_x >= 80) {
    mvprintw((term_y/1.5) + 2, 2, " _    _            _   _              ___  ___                _");
    mvprintw((term_y/1.5) + 3, 2, "| |  | |          | | | |             |  \\/  |               | |");
    mvprintw((term_y/1.5) + 4, 2, "| |  | | ___  __ _| |_| |__   ___ _ __| .  . | __ _  ___  ___| |_ _ __ ___");
    mvprintw((term_y/1.5) + 5, 2, "| |/\\| |/ _ \\/ _` | __| '_ \\ / _ \\ '__| |\\/| |/ _` |/ _ \\/ __| __| '__/ _ \\");
    mvprintw((term_y/1.5) + 6, 2, "\\  /\\  /  __/ (_| | |_| | | |  __/ |  | |  | | (_| |  __/\\__ \\ |_| | | (_) |");
    mvprintw((term_y/1.5) + 7, 2, " \\/  \\/ \\___|\\__,_|\\__|_| |_|\\___|_|  \\_|  |_/\\__,_|\\___||___/\\__|_|  \\___/");
  
  } else {
    mvprintw((term_y/1.5) + 2, 2, " _    ____  ___");
    mvprintw((term_y/1.5) + 3, 2, "| |  | |  \\/  |");
    mvprintw((term_y/1.5) + 4, 2, "| |  | | .  . |");
    mvprintw((term_y/1.5) + 5, 2, "| |/\\| | |\\/| |");
    mvprintw((term_y/1.5) + 6, 2, "\\  /\\  / |  | |");
    mvprintw((term_y/1.5) + 7, 2, " \\/  \\/\\_|  |_/");
  
  }

  refresh();

  
  

  WeatherClient client("Sundbyberg", "Sweden");
  Cities c;
  City c1("Stockholm", "Stockholms Län", "Sverige", 52.123f, 12.234f);
  City c2("Sundbyberg", "Stockholms Län", "Sverige", 52.123f, 12.234f);
  City c3("Malmö", "Skåne Län", "Sverige", 13.456f, 54.234f);

  c.add(c1);
  c.add(c2);
  c.add(c3);

  // c.listCities();

  client.setCityData(c1.getName(), c1.getCountry(), c1.getLat(), c1.getLon());
  client.getWeather();
  // std::cout << client.getTemp() << client.getTemperature_unit() << std::endl;
  
  char input;
  while (input = wgetch(main)) //includes refresh
  {
    switch (input)
    {
    case 'c':
      resetMenu(main);
      wattron(main, COLOR_PAIR(2));
      mvwprintw(main, 0, 2, "[City]");
      wattroff(main, COLOR_PAIR(2));
      break;

    case 'w':
      resetMenu(main);
      wattron(main, COLOR_PAIR(2));
      mvwprintw(main, 0, 10, "[Weather]");
      wattroff(main, COLOR_PAIR(2));
      break;

    case 's':
      resetMenu(main);
      wattron(main, COLOR_PAIR(2));
      mvwprintw(main, 0, 21, "[Settings]");
      wattroff(main, COLOR_PAIR(2));
      break;
    
    default:
    resetMenu(main);
      break;
    }
  }
  



  endwin();
  return 0; 
}
