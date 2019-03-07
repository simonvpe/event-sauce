#pragma once
#include "units.hpp"
#include <immer/box.hpp>
#include <immer/flex_vector.hpp>
#include <immer/vector.hpp>
#include <optional>

struct QuadTree {

  struct Entity {
    tensor<meter_t> position;
    EntityId identifier;
  };

  struct BoundingBox {
    tensor<meter_t> center;
    meter_t half_dimension;

    bool contains(tensor<meter_t> position) const {
      return (position.x >= center.x - half_dimension &&
              position.x <= center.x + half_dimension &&
              position.y >= center.y - half_dimension &&
              position.y <= center.y + half_dimension);
    }

    bool intersects(const BoundingBox &other) const {
      const auto x = center.x;
      const auto y = center.y;
      const auto w = half_dimension;
      return !(other.center.x - other.half_dimension > x + w ||
               other.center.x + other.half_dimension < x - w ||
               other.center.y - other.half_dimension > y + w ||
               other.center.y + other.half_dimension < y - w);
    }
  };

  BoundingBox boundary;
  int capacity;
  bool subdivided = false;
  immer::flex_vector<Entity> entities;
  std::optional<immer::box<QuadTree>> northwest;
  std::optional<immer::box<QuadTree>> northeast;
  std::optional<immer::box<QuadTree>> southwest;
  std::optional<immer::box<QuadTree>> southeast;
};

immer::box<QuadTree> make_qtree(QuadTree::BoundingBox boundary, int capacity) {
  immer::box<QuadTree> qtree;
  return qtree.update([boundary = std::move(boundary), capacity](auto qtree) {
    qtree.boundary = std::move(boundary);
    qtree.capacity = capacity;
    return qtree;
  });
}

immer::box<QuadTree> subdivide(immer::box<QuadTree> qtree) {
  return qtree.update([](auto qtree) {
    const auto x = qtree.boundary.center.x;
    const auto y = qtree.boundary.center.y;
    const auto w = qtree.boundary.half_dimension;
    const auto c = qtree.capacity;
    qtree.northwest = make_qtree({x - w / 2.0f, y - w / 2.0f, w / 2.0f}, c);
    qtree.northeast = make_qtree({x + w / 2.0f, y - w / 2.0f, w / 2.0f}, c);
    qtree.southwest = make_qtree({x - w / 2.0f, y + w / 2.0f, w / 2.0f}, c);
    qtree.southeast = make_qtree({x + w / 2.0f, y + w / 2.0f, w / 2.0f}, c);
    qtree.subdivided = true;
    return qtree;
  });
}

// Insert an entity into the qtree
immer::box<QuadTree> insert(immer::box<QuadTree> qtree, EntityId identifier,
                            tensor<meter_t> position);

// Query the qtree
immer::vector<QuadTree::Entity> query(immer::box<QuadTree> qtree,
                                      QuadTree::BoundingBox range);

////////////////////////////////////////////////////////////////////////////////
// Implementation details follow
////////////////////////////////////////////////////////////////////////////////

namespace detail {
std::optional<immer::box<QuadTree>> insert_impl(immer::box<QuadTree> qtree,
                                                EntityId identifier,
                                                tensor<meter_t> position) {
  // Ignore objects that do not belong to this quad tree
  if (!qtree->boundary.contains(position)) {
    return {};
  }

  // If there is space in this quad tree and if it doesn't have subdivisions,
  // add the object here
  if (qtree->entities.size() < qtree->capacity) {
    return qtree.update([identifier, position](auto qtree) {
      qtree.entities = qtree.entities.push_back({position, identifier});
      return qtree;
    });
  }

  if (!qtree->subdivided) {
    qtree = subdivide(qtree);
  }

  bool done = false;

  qtree = qtree.update([identifier, position, &done](auto qtree) {
    if (auto nw = insert_impl(*qtree.northwest, identifier, position)) {
      qtree.northwest = nw;
      done = true;
    }
    return qtree;
  });
  if (done) {
    return qtree;
  }

  qtree = qtree.update([identifier, position, &done](auto qtree) {
    if (auto ne = insert_impl(*qtree.northeast, identifier, position)) {
      qtree.northeast = ne;
      done = true;
    }
    return qtree;
  });
  if (done) {
    return qtree;
  }

  qtree = qtree.update([identifier, position, &done](auto qtree) {
    if (auto sw = insert_impl(*qtree.southwest, identifier, position)) {
      qtree.southwest = sw;
      done = true;
    }
    return qtree;
  });
  if (done) {
    return qtree;
  }

  qtree = qtree.update([identifier, position, &done](auto qtree) {
    if (auto se = insert_impl(*qtree.southeast, identifier, position)) {
      qtree.southeast = se;
      done = true;
    }
    return qtree;
  });
  if (!done) {
    throw std::runtime_error{"Failed to insert into qtree"};
  }

  return qtree;
}

std::optional<immer::box<QuadTree>> remove_impl(immer::box<QuadTree> qtree,
                                                EntityId identifier,
                                                QuadTree::BoundingBox range) {
  if (!qtree->boundary.intersects(range)) {
    return {};
  }
  auto i = 0;
  for (const auto &entity : qtree->entities) {
    if (entity.identifier == identifier) {
      return qtree.update([&i](auto qtree) {
        qtree.entities = qtree.entities.erase(i);
        return qtree;
      });
    }
    ++i;
  }
  if (qtree->northwest) {
    if (auto nw = remove_impl(*qtree->northwest, identifier, range)) {
      return qtree.update([nw](auto qtree) {
        qtree.northwest = nw;
        return qtree;
      });
    }
  }
  if (qtree->northeast) {
    if (auto ne = remove_impl(*qtree->northeast, identifier, range)) {
      return qtree.update([ne](auto qtree) {
        qtree.northeast = ne;
        return qtree;
      });
    }
  }
  if (qtree->southwest) {
    if (auto sw = remove_impl(*qtree->southwest, identifier, range)) {
      return qtree.update([sw](auto qtree) {
        qtree.southwest = sw;
        return qtree;
      });
    }
  }
  if (qtree->southeast) {
    if (auto se = remove_impl(*qtree->southeast, identifier, range)) {
      return qtree.update([se](auto qtree) {
        qtree.southeast = se;
        return qtree;
      });
    }
  }
  return {};
}
} // namespace detail

immer::box<QuadTree> insert(immer::box<QuadTree> qtree, EntityId identifier,
                            tensor<meter_t> position) {
  return *detail::insert_impl(qtree, identifier, position);
}

immer::vector<QuadTree::Entity> query(immer::box<QuadTree> qtree,
                                      QuadTree::BoundingBox range) {
  immer::vector<QuadTree::Entity> entities;
  if (!qtree->boundary.intersects(range)) {
    return entities;
  }
  for (const auto &entity : qtree->entities) {
    if (range.contains(entity.position)) {
      entities = entities.push_back(entity);
    }
  }
  if (qtree->northwest) {
    for (const auto &entity : query(*qtree->northwest, range)) {
      entities = entities.push_back(entity);
    }
  }
  if (qtree->northeast) {
    for (const auto &entity : query(*qtree->northeast, range)) {
      entities = entities.push_back(entity);
    }
  }
  if (qtree->southwest) {
    for (const auto &entity : query(*qtree->southwest, range)) {
      entities = entities.push_back(entity);
    }
  }
  if (qtree->southeast) {
    for (const auto &entity : query(*qtree->southeast, range)) {
      entities = entities.push_back(entity);
    }
  }
  return entities;
}

immer::box<QuadTree> remove(immer::box<QuadTree> qtree, EntityId identifier,
                            QuadTree::BoundingBox range) {
  if (auto result = detail::remove_impl(qtree, identifier, std::move(range))) {
    return *result;
  }
  return std::move(qtree);
}

immer::box<QuadTree> move(immer::box<QuadTree> qtree, EntityId identifier,
                          tensor<meter_t> destination, meter_t search_width) {
  auto range = QuadTree::BoundingBox{destination, search_width};
  auto removed = detail::remove_impl(qtree, identifier, range);
  if (removed) {
    return insert(*removed, identifier, destination);
  }
  return qtree;
}
