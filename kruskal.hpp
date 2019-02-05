
#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>
#include <array>

template<int Width, int Height, int K>
auto kruskal() {
  static constexpr auto N = 1;
  static constexpr auto S = 2;
  static constexpr auto E = 4;
  static constexpr auto W = 8;

  const auto dx = [](auto dir) {
    switch (dir) {
    case N:
      return 0;
    case S:
      return 0;
    case E:
      return 1;
    case W:
      return -1;
    default:
      assert(false);
    }
  };

  const auto dy = [](auto dir) {
    switch (dir) {
    case N:
      return -1;
    case S:
      return 1;
    case E:
      return 0;
    case W:
      return 0;
    default:
      assert(false);
    }
  };

  const auto opposite = [](auto dir) {
    switch (dir) {
    case N:
      return S;
    case S:
      return N;
    case E:
      return W;
    case W:
      return E;
    default:
      assert(false);
    }
  };

  struct Tree {
    Tree &root() { return parent ? parent->root() : *this; }
    bool connected(Tree &other) { return &root() == &other.root(); }
    void connect(Tree &other) { other.root().parent = this; }
    Tree *parent = nullptr;
  };

  struct Edge {
    int x, y, direction;
  };

  auto grid = std::vector<std::vector<int>>(Height, std::vector<int>(Width, 0));
  auto sets =
      std::vector<std::vector<Tree>>(Height, std::vector<Tree>(Width, Tree{}));

  auto edges = std::vector<Edge>{};

  for (auto y = 0; y < Height; ++y) {
    for (auto x = 0; x < Width; ++x) {
      if (y > 0) {
        edges.push_back({x, y, N});
      }
      if (x > 0) {
        edges.push_back({x, y, W});
      }
    }
  }

  std::random_shuffle(edges.begin(), edges.end());

  while (!edges.empty()) {
    const auto edge = edges.back();
    edges.pop_back();
    const auto nx = edge.x + dx(edge.direction);
    const auto ny = edge.y + dy(edge.direction);
    auto &set1 = sets[edge.y][edge.x];
    auto &set2 = sets[ny][nx];

    if (!set1.connected(set2)) {
      set1.connect(set2);
      grid[edge.y][edge.x] |= edge.direction;
      grid[ny][nx] |= opposite(edge.direction);
    }
  }

  using Row =std::array<bool, K*Width>;
  using Grid = std::array<Row, K*Height>;
  auto result = Grid{};

  for(auto x = 0 ; x < K*Width ; ++x) {
    result[0][x] = true;
    result[K*Height - 1][x] = true;
  }
  for(auto y = 0 ; y < K*Height ; ++y) {
    result[y][0] = true;
    result[y][K*Width - 1] = true;
  }
  return result;
}
