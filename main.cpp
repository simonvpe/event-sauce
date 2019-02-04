#include "aggregates/player.hpp"
#include "event-sauce.hpp"
#include <chrono>
#include <iostream>

/*******************************************************************************
 ** Example
 *******************************************************************************/

#include <SFML/Graphics.hpp>
#include <memory>
#include <sstream>

/*******************************************************************************
 ** PlayerProjection
 *******************************************************************************/
template <typename Gui> struct PlayerProjection {
  constexpr static auto execute = event_sauce::disabled;
  constexpr static auto apply = event_sauce::disabled;
  struct state_type {};

  //////////////////////////////////////////////////////////////////////////////
  // Apply PositionChanged
  static void project(Gui &gui, const PositionChanged &event) {
    static constexpr auto scale_factor = 100.0f;
    auto ship = sf::CircleShape{80.f, 3};
    ship.setOrigin(80.f, 80.f);
    degree_t rotation = event.rotation;
    ship.setRotation(rotation.to<float>() + 90.0f);
    ship.move({event.position.x.to<float>() * scale_factor,
               event.position.y.to<float>() * scale_factor});
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
    if (!font.loadFromFile("/usr/share/doc/dbus-python/_static/fonts/"
                           "RobotoSlab/roboto-slab-v7-regular.ttf")) {
      throw std::runtime_error{"Failed to load fonts!"};
    }
  }
};

int main() {
  using namespace std::chrono_literals;
  Gui gui;
  auto ctx = event_sauce::make_context<Player<0>, PlayerProjection<Gui>>(gui);

  auto t = std::chrono::high_resolution_clock::now();
  while (gui.window.isOpen()) {
    sf::Event event;
    while (gui.window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        gui.window.close();
      }
      if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Up) {
          ctx.dispatch(ActivateMainThruster{});
        }
        if (event.key.code == sf::Keyboard::Left) {
          ctx.dispatch(ActivateRightThruster{});
        }
        if (event.key.code == sf::Keyboard::Right) {
          ctx.dispatch(ActivateLeftThruster{});
        }
      }
      if (event.type == sf::Event::KeyReleased) {
        if (event.key.code == sf::Keyboard::Up) {
          ctx.dispatch(DeactivateMainThruster{});
        }
        if (event.key.code == sf::Keyboard::Left) {
          ctx.dispatch(DeactivateRightThruster{});
        }
        if (event.key.code == sf::Keyboard::Right) {
          ctx.dispatch(DeactivateLeftThruster{});
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
