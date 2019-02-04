#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

void kruskal(int width, int height) {
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

  auto grid = std::vector<std::vector<int>>(height, std::vector<int>(width, 0));
  auto sets =
      std::vector<std::vector<Tree>>(height, std::vector<Tree>(width, Tree{}));

  auto edges = std::vector<Edge>{};

  for (auto y = 0; y < height; ++y) {
    for (auto x = 0; x < width; ++x) {
      if (y > 0) {
        edges.push_back({x, y, N});
      }
      if (x > 0) {
        edges.push_back({x, y, W});
      }
    }
  }

  std::random_shuffle(edges.begin(), edges.end());

  const auto display = [](const auto &grid) {
    std::cout << " ";
    for (auto i = 0; i < grid[0].size() * 2 - 1; ++i) {
      std::cout << "_";
    }
    std::cout << std::endl;
    for (auto y = 0; y < grid.size(); ++y) {
      const auto &row = grid[y];
      std::cout << "|";
      for (auto x = 0; x < row.size(); ++x) {
        const auto &cell = row[x];
        if (cell == 0) {
          std::cout << "*";
        }
        std::cout << ((cell & S) ? " " : "_");
        if (cell & E) {
          std::cout << ((cell | row[x + 1] & S) ? " " : "_");
        } else {
          std::cout << "|";
        }
        if (cell == 0) {
          std::cout << "*";
        }
      }
      std::cout << "\n";
    }
  };

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
      display(grid);
    }
  }
}
