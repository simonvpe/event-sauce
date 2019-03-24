#include "aggregates/collider.hpp"
#include "aggregates/map.hpp"
#include "aggregates/player.hpp"
#include "aggregates/rigid_body.hpp"
#include "aggregates/time.hpp"
#include "vendor/event-sauce.hpp"
#include <chrono>
#include <imgui-SFML.h>
#include <imgui.h>
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
  static void project(Gui &gui, const Entity::PositionChanged &event) {
    sf::Sprite ship;
    ship.setTexture(gui.shipTexture);
    {
      const auto bounds = ship.getLocalBounds();
      const float scale_x = 1_m / bounds.width * scale_factor;
      const float scale_y = 1_m / bounds.height * scale_factor;
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

  static void project(Gui &gui, const Entity::RotationChanged &event) {
    degree_t angle = event.rotation;
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
  static void project(Gui &gui, const Entity::PositionChanged &event) {
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
    window.setVerticalSyncEnabled(true);
    ImGui::SFML::Init(window);
    window.resetGLStates();

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

#include "vendor/serialize_std_variant.hpp"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <fstream>

template <typename Archive>
void serialize(Archive &ar, Player::Created &evt, const unsigned int version) {
  ar &evt.player_id;
}

int main() {
  using namespace std::chrono_literals;

  {
    std::variant<int, Player::Created> evt = Player::Created{12};
    std::ofstream f{"file"};
    boost::archive::text_oarchive oa{f};
    oa << evt;
  }
  {
    std::ifstream f{"file"};
    boost::archive::text_iarchive ia{f};
    std::variant<int, Player::Created> evt;
    ia >> evt;
    std::cout << "IS PLAYER::  " << std::get<Player::Created>(evt).player_id << std::endl;
  }
  Gui gui;

  auto ctx =
      event_sauce::make_context<Player, PlayerProjection<Gui>,
                                CameraProjection<Gui>, Map, MapProjection<Gui>,
                                Entity, Time, RigidBody, Collider>(gui);
  const auto maze = kruskal<10, 10, 10>();

  ctx.dispatch(CreateMap{std::move(maze)});
  ctx.dispatch(Player::Create{999});
  ctx.dispatch(Player::Create{111});

  char windowTitle[256] = "ImGui + SFML = <3";
  gui.window.setTitle(windowTitle);

  auto t = std::chrono::high_resolution_clock::now();
  sf::Clock deltaClock;
  while (gui.window.isOpen()) {
    sf::Event event;
    while (gui.window.pollEvent(event)) {
      ImGui::SFML::ProcessEvent(event);
      if (event.type == sf::Event::Closed) {
        gui.window.close();
      }
    }

    if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
      ctx.dispatch(Player::ActivateThruster{999});
    }

    ImGui::SFML::Update(gui.window, deltaClock.restart());
    ImGui::Begin("Sample window");
    ImGui::InputText("Window title", windowTitle, 255);

    if (ImGui::Button("Update window title")) {
      // this code gets if user clicks on the button
      // yes, you could have written if(ImGui::InputText(...))
      // but I do this to show how buttons work :)
      gui.window.setTitle(windowTitle);
    }
    ImGui::End(); // end window

    ImGui::Begin("Window 2");
    ImGui::End();

    gui.window.clear(sf::Color::Black);
    const auto now = std::chrono::high_resolution_clock::now();
    ctx.dispatch(Tick{0, now - t});
    t = now;

    const auto mouse_position =
        gui.window.mapPixelToCoords(sf::Mouse::getPosition(gui.window));
    const auto ship_position = gui.ship_position;
    const auto look_vector = mouse_position - ship_position;
    const auto angle = radian_t{std::atan2(look_vector.y, look_vector.x)};
    ctx.dispatch(Player::SetRotation{999, angle});
    gui.draw();
    ImGui::SFML::Render(gui.window);
    gui.window.display();
  }

  return 0;
}
