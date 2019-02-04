#include "aggregates/player.hpp"
#include "vendor/event-sauce.hpp"
#include <chrono>
#include <iostream>

/*******************************************************************************
 ** Example
 *******************************************************************************/

#include "kruskal.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <sstream>

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
    /*
    auto ship = sf::CircleShape{80.f, 3};
    ship.setOrigin(80.f, 80.f);*/
    auto ship = sf::Sprite{};
    ship.setTexture(gui.shipTexture);
    {
      const auto bounds = ship.getLocalBounds();
      const float scale_x = event.size.x / bounds.width * scale_factor;
      const float scale_y = event.size.y / bounds.height * scale_factor;
      ship.scale(scale_x, scale_y);
    }
    {
      const auto bounds = ship.getLocalBounds();
      ship.setOrigin(bounds.width/2, bounds.height/2);
    }
    {
      const degree_t rotation = event.rotation;
      ship.setRotation(rotation.to<float>() + 90.0f);
    }
    {
      const float x = event.position.x * scale_factor;
      const float y = event.position.y * scale_factor;
      ship.move({x, y});
    }
    gui.window.draw(ship);
  }
};

/*******************************************************************************
 ** CameraProjection
 *******************************************************************************/
template <int Id, typename Gui> struct CameraProjection {
  constexpr static auto execute = event_sauce::disabled;
  constexpr static auto apply = event_sauce::disabled;
  struct state_type {};

  //////////////////////////////////////////////////////////////////////////////
  // Apply PositionChanged
  static void project(Gui &gui, const PositionChanged<Id> &event) {
    static constexpr auto scale_factor = 100.0f;
    const float x = event.position.x * scale_factor;
    const float y = event.position.y * scale_factor;

    auto view = gui.window.getDefaultView();
    view.setCenter({x, y});
    gui.window.setView(view);
  }
};

/*******************************************************************************
 ** main
 *******************************************************************************/
struct Gui {
  sf::RenderWindow window;
  sf::Font font;
  sf::Texture shipTexture;
  Gui() : window{sf::VideoMode(800, 600), "My window"} {
    // if (!font.loadFromFile("/usr/share/doc/dbus-python/_static/fonts/"
    //                        "RobotoSlab/roboto-slab-v7-regular.ttf")) {
    //   throw std::runtime_error{"Failed to load fonts!"};
    // }
    if (!shipTexture.loadFromFile("../spaceship.png")) {
      throw std::runtime_error{"Failed to load sprite"};
    }
  }
};

int main() {
  using namespace std::chrono_literals;

  const auto maze = kruskal(10, 10);
  Gui gui;

  using Player_0 = Player<0>;
  using PlayerProjection_0 = PlayerProjection<0, Gui>;
  using CameraProjection_0 = CameraProjection<0, Gui>;

  auto ctx = event_sauce::make_context<Player_0, PlayerProjection_0,
                                       CameraProjection_0>(gui);

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

    for (const auto &coords : maze) {
      const float x0 = std::get<0>(coords).first * 500;
      const float y0 = std::get<0>(coords).second * 500;
      const float x1 = std::get<1>(coords).first * 500;
      const float y1 = std::get<1>(coords).second * 500;
      sf::VertexArray line(sf::Lines, 2);
      line[0].position = {x0, y0};
      line[0].color = sf::Color::White;
      line[1].position = {x1, y1};
      line[1].color = sf::Color::White;
      gui.window.draw(line);
    }

    const auto now = std::chrono::high_resolution_clock::now();
    ctx.dispatch(Tick{now - t});
    t = now;

    gui.window.display();
  }

  return 0;
}
