#pragma once
#include "../vendor/units.h"

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

template <typename T> struct tensor {
  T x, y;
  tensor operator+(const tensor &other) { return {x + other.x, y + other.y}; }
};
