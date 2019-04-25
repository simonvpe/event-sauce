#pragma once
#include <glm/vec2.hpp>

namespace physics {
struct entity
{
  using id_type = int;

  struct create
  {};

  struct move
  {
    id_type id;
    glm::vec2 distance;
  };
};
}
