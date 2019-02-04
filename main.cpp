#include "aggregates/player.hpp"
#include "vendor/event-sauce.hpp"
#include <chrono>
#include <iostream>

/*******************************************************************************
 ** Example
 *******************************************************************************/

#include <SFML/Graphics.hpp>
#include <memory>
#include <sstream>
#include "kruskal.hpp"

/*******************************************************************************
 ** PlayerProjection
 *******************************************************************************/
template <int Id, typename Gui> struct PlayerProjection {
  constexpr static auto execute = event_sauce::disabled;
  constexpr static auto apply = event_sauce::disabled;
  struct state_type {};

  //////////////////////////////////////////////////////////////////////////////
  // Apply PositionChanged
  static void project(Gui &gui, const PositionChanged<Id> &event) {
    static constexpr auto scale_factor = 100.0f;
    auto ship = sf::CircleShape{80.f, 3};
    ship.setOrigin(80.f, 80.f);
    degree_t rotation = event.rotation;
    ship.setRotation(rotation.to<float>() + 90.0f);
    const float x = event.position.x * scale_factor;
    const float y = event.position.y * scale_factor;
    ship.move({x, y});
    gui.window.draw(ship);
  }
};

/*******************************************************************************
 ** main
 *******************************************************************************/
struct Gui {
  sf::RenderWindow window;
  sf::Font font;
  Gui() : window{sf::VideoMode(800, 600), "My window"} {
    // if (!font.loadFromFile("/usr/share/doc/dbus-python/_static/fonts/"
    //                        "RobotoSlab/roboto-slab-v7-regular.ttf")) {
    //   throw std::runtime_error{"Failed to load fonts!"};
    // }
  }
};

int main() {
  using namespace std::chrono_literals;

  kruskal(10, 10);
  Gui gui;

  using Player_0 = Player<0>;
  using PlayerProjection_0 = PlayerProjection<0, Gui>;
  using Player_1 = Player<1>;
  using PlayerProjection_1 = PlayerProjection<1, Gui>;

  auto ctx = event_sauce::make_context<Player_0, Player_1, PlayerProjection_0,
                                       PlayerProjection_1>(gui);

  auto t = std::chrono::high_resolution_clock::now();
  while (gui.window.isOpen()) {
    sf::Event event;
    while (gui.window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        gui.window.close();
      }
      if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Up) {
          ctx.dispatch(ActivateMainThruster<0>{});
        }
        if (event.key.code == sf::Keyboard::Left) {
          ctx.dispatch(ActivateRightThruster<0>{});
        }
        if (event.key.code == sf::Keyboard::Right) {
          ctx.dispatch(ActivateLeftThruster<0>{});
        }
      }
      if (event.type == sf::Event::KeyReleased) {
        if (event.key.code == sf::Keyboard::Up) {
          ctx.dispatch(DeactivateMainThruster<0>{});
        }
        if (event.key.code == sf::Keyboard::Left) {
          ctx.dispatch(DeactivateRightThruster<0>{});
        }
        if (event.key.code == sf::Keyboard::Right) {
          ctx.dispatch(DeactivateLeftThruster<0>{});
        }
      }
    }

    gui.window.clear(sf::Color::Black);
    const auto now = std::chrono::high_resolution_clock::now();
    ctx.dispatch(Tick{now - t});
    t = now;
    gui.window.display();
  }

  return 0;
}
