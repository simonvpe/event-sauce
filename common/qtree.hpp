#pragma once
#include "units.hpp"
#include <immer/box.hpp>
#include <immer/flex_vector.hpp>
#include <immer/vector.hpp>
#include <optional>

template <typename T> struct QuadTreeImpl;
template <typename T> using quad_tree = immer::box<QuadTreeImpl<T>>;

template <typename T> struct QuadTreeImpl {

  struct Entity {
    tensor<meter_t> position;
    T payload;
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
  std::optional<immer::box<QuadTreeImpl>> northwest;
  std::optional<immer::box<QuadTreeImpl>> northeast;
  std::optional<immer::box<QuadTreeImpl>> southwest;
  std::optional<immer::box<QuadTreeImpl>> southeast;

  QuadTreeImpl() = default;
  QuadTreeImpl(meter_t x, meter_t y, meter_t half_dimension, int capacity)
      : boundary{x, y, half_dimension}, capacity{capacity} {}
};

template <typename T> quad_tree<T> subdivide(quad_tree<T> qtree) {
  return qtree.update([](auto qtree) {
    const auto x = qtree.boundary.center.x;
    const auto y = qtree.boundary.center.y;
    const auto w = qtree.boundary.half_dimension;
    const auto c = qtree.capacity;
    qtree.northwest = quad_tree<T>(x - w / 2.0f, y - w / 2.0f, w / 2.0f, c);
    qtree.northeast = quad_tree<T>(x + w / 2.0f, y - w / 2.0f, w / 2.0f, c);
    qtree.southwest = quad_tree<T>(x - w / 2.0f, y + w / 2.0f, w / 2.0f, c);
    qtree.southeast = quad_tree<T>(x + w / 2.0f, y + w / 2.0f, w / 2.0f, c);
    qtree.subdivided = true;
    return qtree;
  });
}

// Insert an entity into the qtree
template <typename T>
quad_tree<T> insert(quad_tree<T> qtree, T payload, tensor<meter_t> position);

// Query the qtree
template <typename T>
immer::vector<typename quad_tree<T>::Entity>
query(quad_tree<T> qtree, typename quad_tree<T>::BoundingBox range);

////////////////////////////////////////////////////////////////////////////////
// Implementation details follow
////////////////////////////////////////////////////////////////////////////////

namespace detail {
template <typename T>
std::optional<quad_tree<T>> insert_impl(quad_tree<T> qtree, T payload,
                                        tensor<meter_t> position) {
  // Ignore objects that do not belong to this quad tree
  if (!qtree->boundary.contains(position)) {
    return {};
  }

  // If there is space in this quad tree and if it doesn't have subdivisions,
  // add the object here
  if (qtree->entities.size() < qtree->capacity) {
    return qtree.update([payload, position](auto qtree) {
      qtree.entities = qtree.entities.push_back({position, payload});
      return qtree;
    });
  }

  if (!qtree->subdivided) {
    qtree = subdivide(qtree);
  }

  bool done = false;

  qtree = qtree.update([payload, position, &done](auto qtree) {
    if (auto nw = insert_impl(*qtree.northwest, payload, position)) {
      qtree.northwest = nw;
      done = true;
    }
    return qtree;
  });
  if (done) {
    return qtree;
  }

  qtree = qtree.update([payload, position, &done](auto qtree) {
    if (auto ne = insert_impl(*qtree.northeast, payload, position)) {
      qtree.northeast = ne;
      done = true;
    }
    return qtree;
  });
  if (done) {
    return qtree;
  }

  qtree = qtree.update([payload, position, &done](auto qtree) {
    if (auto sw = insert_impl(*qtree.southwest, payload, position)) {
      qtree.southwest = sw;
      done = true;
    }
    return qtree;
  });
  if (done) {
    return qtree;
  }

  qtree = qtree.update([payload, position, &done](auto qtree) {
    if (auto se = insert_impl(*qtree.southeast, payload, position)) {
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

template <typename T, typename F>
std::optional<quad_tree<T>>
remove_impl(quad_tree<T> qtree, typename QuadTreeImpl<T>::BoundingBox range,
            F &&predicate) {
  if (!qtree->boundary.intersects(range)) {
    return {};
  }
  auto i = 0;
  for (const auto &entity : qtree->entities) {
    if (predicate(entity.payload)) {
      return qtree.update([&i](auto qtree) {
        qtree.entities = qtree.entities.erase(i);
        return qtree;
      });
    }
    ++i;
  }
  if (qtree->northwest) {
    if (auto nw = remove_impl(*qtree->northwest, range, predicate)) {
      return qtree.update([nw](auto qtree) {
        qtree.northwest = nw;
        return qtree;
      });
    }
  }
  if (qtree->northeast) {
    if (auto ne = remove_impl(*qtree->northeast, range, predicate)) {
      return qtree.update([ne](auto qtree) {
        qtree.northeast = ne;
        return qtree;
      });
    }
  }
  if (qtree->southwest) {
    if (auto sw = remove_impl(*qtree->southwest, range, predicate)) {
      return qtree.update([sw](auto qtree) {
        qtree.southwest = sw;
        return qtree;
      });
    }
  }
  if (qtree->southeast) {
    if (auto se = remove_impl(*qtree->southeast, range, predicate)) {
      return qtree.update([se](auto qtree) {
        qtree.southeast = se;
        return qtree;
      });
    }
  }
  return {};
}
} // namespace detail

template <typename T>
quad_tree<T> insert(quad_tree<T> qtree, T payload, tensor<meter_t> position) {
  return *detail::insert_impl(qtree, payload, position);
}

template <typename T>
immer::vector<typename QuadTreeImpl<T>::Entity>
query(quad_tree<T> qtree, typename QuadTreeImpl<T>::BoundingBox range) {
  immer::vector<typename QuadTreeImpl<T>::Entity> entities;
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

template <typename T, typename F>
quad_tree<T> remove(quad_tree<T> qtree,
                    typename QuadTreeImpl<T>::BoundingBox range,
                    F &&predicate) {
  if (auto result =
          detail::remove_impl(qtree, std::move(range), std::move(predicate))) {
    return *result;
  }
  return std::move(qtree);
}

template <typename T, typename F>
quad_tree<T> move(quad_tree<T> qtree, tensor<meter_t> destination,
                  meter_t search_width, F &&predicate) {
  T pl;
  auto pred = [&pl, &predicate](const auto &payload) mutable {
    // store payload for later use
    if (predicate(payload)) {
      pl = payload;
      return true;
    }
    return false;
  };
  auto range = typename QuadTreeImpl<T>::BoundingBox{destination, search_width};
  if (auto removed = detail::remove_impl(qtree, range, std::move(pred))) {
    return insert(*removed, std::move(pl), destination);
  }
  return qtree;
}
