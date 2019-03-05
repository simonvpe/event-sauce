#include "aggregates/collision.hpp"
#include "aggregates/map.hpp"
#include "aggregates/mouse_aim.hpp"
#include "aggregates/player.hpp"
#include "aggregates/rigid_body.hpp"
#include "aggregates/time.hpp"
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

static constexpr auto scale_factor = 100.0f;

/*******************************************************************************
 ** PlayerProjection
 *******************************************************************************/
template <typename Gui> struct PlayerProjection {
  constexpr static auto execute = event_sauce::disabled;
  constexpr static auto apply = event_sauce::disabled;
  constexpr static auto process = event_sauce::disabled;
  struct state_type {};

  //////////////////////////////////////////////////////////////////////////////
  // Apply PositionChanged
  static void project(Gui &gui, const PositionChanged &event) {
    sf::Sprite ship;
    ship.setTexture(gui.shipTexture);
    {
      const auto bounds = ship.getLocalBounds();
      const float scale_x = event.size.x / bounds.width * scale_factor;
      const float scale_y = event.size.y / bounds.height * scale_factor;
      ship.scale(scale_x, scale_y);
    }
    {
      const auto bounds = ship.getLocalBounds();
      ship.setOrigin(bounds.width / 2, bounds.height / 2);
    }
    {
      const float x = event.position.x * scale_factor;
      const float y = event.position.y * scale_factor;
      ship.move({x, y});
      gui.ship_position = {x, y};
    }
    { ship.setRotation(gui.ship_rotation); }
    gui.window.draw(ship);

    for (const auto &row : gui.tiles) {
      for (const auto &sprite : row) {
        gui.window.draw(sprite);
      }
    }
  }

  static void project(Gui &gui, const ShipRotationChanged &event) {
    degree_t angle = event.angle;
    gui.ship_rotation = angle.to<float>() + 90.0f;
  }
};

/*******************************************************************************
 ** CameraProjection
 *******************************************************************************/
template <typename Gui> struct CameraProjection {
  constexpr static auto execute = event_sauce::disabled;
  constexpr static auto apply = event_sauce::disabled;
  constexpr static auto process = event_sauce::disabled;
  struct state_type {};

  //////////////////////////////////////////////////////////////////////////////
  // Apply PositionChanged
  static void project(Gui &gui, const PositionChanged &event) {
    const float x = event.position.x * scale_factor;
    const float y = event.position.y * scale_factor;

    auto view = gui.window.getDefaultView();
    view.setCenter({x, y});
    gui.window.setView(view);
  }
};

/*******************************************************************************
 ** MapProjection
 *******************************************************************************/
template <typename Gui> struct MapProjection {
  constexpr static auto execute = event_sauce::disabled;
  constexpr static auto apply = event_sauce::disabled;
  constexpr static auto process = event_sauce::disabled;
  struct state_type {};

  //////////////////////////////////////////////////////////////////////////////
  // Apply MapUpdated
  static void project(Gui &gui, const MapUpdated &event) {
    gui.map.clear();
    for (const auto &coords : event.grid) {
      for (auto y = 0; y < event.grid.size(); ++y) {
        const auto &row = event.grid[y];
        for (auto x = 0; x < row.size(); ++x) {
          const auto &cell = row[x];
          if (cell) {
            const float px = CreateMap::cell_width * scale_factor * x;
            const float py = CreateMap::cell_width * scale_factor * y;
            sf::RectangleShape sprite(
                {(float)CreateMap::cell_width * scale_factor,
                 (float)CreateMap::cell_width * scale_factor});
            sprite.move({px, py});
            gui.tiles[y][x] = sprite;
          }
        }
      }
    }
  }
};

/*******************************************************************************
 ** main
 *******************************************************************************/
struct Gui {
  sf::RenderWindow window;
  sf::Font font;
  sf::Texture shipTexture;
  std::vector<sf::VertexArray> map;
  sf::Sprite ship;
  double ship_rotation = 0.0f;
  sf::Vector2<float> ship_position;
  std::array<std::array<sf::RectangleShape, 100>, 100> tiles;

  Gui() : window{sf::VideoMode(800, 600), "My window"} {
    // if (!font.loadFromFile("/usr/share/doc/dbus-python/_static/fonts/"
    //                        "RobotoSlab/roboto-slab-v7-regular.ttf")) {
    //   throw std::runtime_error{"Failed to load fonts!"};
    // }
    if (!shipTexture.loadFromFile("../spaceship.png")) {
      throw std::runtime_error{"Failed to load sprite"};
    }
  }

  void draw() {
    for (const auto &line : map) {
      window.draw(line);
    }
    window.draw(ship);
  }
};

int main() {
  using namespace std::chrono_literals;

  Gui gui;

  auto ctx = event_sauce::make_context<Player, PlayerProjection<Gui>,
                                       CameraProjection<Gui>, Map,
                                       MapProjection<Gui>, Collision, Thruster,
                                       MouseAim, Entity, Time, RigidBody>(gui);
  const auto maze = kruskal<10, 10, 10>();
  /*
std::vector<std::tuple<tensor<meter_t>, tensor<meter_t>>> map;
std::transform(maze.begin(), maze.end(), std::back_inserter(map),
[](const auto &segment) {
const auto p0 = std::get<0>(segment);
const auto p1 = std::get<1>(segment);
auto s0 = tensor<meter_t>{meter_t(p0.first * 10),
                   meter_t(p0.second * 10)};
auto s1 = tensor<meter_t>{meter_t(p1.first * 10),
                   meter_t(p1.second * 10)};
return std::make_tuple(s0, s1);
});
  */
  ctx.dispatch(CreateMap{std::move(maze)});
  ctx.dispatch(CreateEntity{12});
  ctx.dispatch(CreateRigidBody{0, 12, 1_kg});

  auto t = std::chrono::high_resolution_clock::now();
  while (gui.window.isOpen()) {
    sf::Event event;
    while (gui.window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        gui.window.close();
      }
      if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Left) {
          ctx.dispatch(ActivateThruster{0});
        }
      }
      if (event.type == sf::Event::MouseButtonReleased) {
        if (event.mouseButton.button == sf::Mouse::Left) {
          ctx.dispatch(DeactivateThruster{0});
        }
      }
    }

    ctx.dispatch(ApplyForce{0, 12, {10_N, 10_N}});

    gui.window.clear(sf::Color::Black);
    const auto now = std::chrono::high_resolution_clock::now();
    ctx.dispatch(Tick{0, now - t});
    t = now;

    const auto mouse_position =
        gui.window.mapPixelToCoords(sf::Mouse::getPosition(gui.window));
    const auto ship_position = gui.ship_position;
    const auto look_vector = mouse_position - ship_position;
    const auto angle = radian_t{std::atan2(look_vector.y, look_vector.x)};
    ctx.dispatch(SetShipRotation{0, angle});

    gui.draw();
    gui.window.display();
  }

  return 0;
}
