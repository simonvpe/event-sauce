#pragma once
#include "../vendor/units.h"
#include <iostream>

using namespace units::literals;
using namespace units::angle;
using namespace units::angular_velocity;
using namespace units::acceleration;
using namespace units::force;
using namespace units::time;
using namespace units::math;
using namespace units::mass;
using namespace units::velocity;
using namespace units::length;
using namespace units::torque;

using mps_sq_t = meters_per_second_squared_t;
using mps_t = meters_per_second_t;
using kilogram_meters_squared_t = decltype(1.0_kg * 1.0_sq_m);
using radians_per_second_squared_t = decltype(1.0_rad / 1.0_s / 1.0_s);

template<typename T>
struct tensor
{
  T x, y;
  tensor operator+(const tensor& other) { return { x + other.x, y + other.y }; }
};

template<typename T, typename U>
auto
operator/(const tensor<T>& lhs, const U& scalar) -> tensor<decltype(std::declval<T>() / std::declval<U>())>
{
  return { lhs.x / scalar, lhs.y / scalar };
}

template<typename T, typename U>
auto operator*(const tensor<T>& lhs, const U& scalar) -> tensor<decltype(std::declval<T>() * std::declval<U>())>
{
  return { lhs.x * scalar, lhs.y * scalar };
}

template<typename T>
auto
operator-(const tensor<T>& lhs, const tensor<T>& rhs) -> tensor<T>
{
  return { lhs.x - rhs.x, lhs.y - rhs.y };
}

template<typename T>
auto
operator+(const tensor<T>& lhs, const tensor<T>& rhs) -> tensor<T>
{
  return { lhs.x + rhs.x, lhs.y + rhs.y };
}

template<typename T>
auto
operator+=(tensor<T>& lhs, const tensor<T>& rhs) -> tensor<T>&
{
  lhs.x += rhs.x;
  lhs.y += rhs.y;
  return lhs;
}

template<typename T>
std::ostream&
operator<<(std::ostream& ost, const tensor<T>& t)
{
  ost << "(" << t.x << ", " << t.y << ")";
  return ost;
}

template<typename T>
struct rectangle
{
  tensor<T> top_left, bottom_right;
};
