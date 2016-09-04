#pragma once

#include <pebble.h>

static const GPathInfo DIRECTION_POINTER_POINTS = {
  5, (GPoint []){
    {-5, -75 },
    { 5, -75 },
    { 5, -75 },
    { 0, -85 },
    {-5, -75 }
  }
};