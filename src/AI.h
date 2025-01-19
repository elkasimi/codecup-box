#pragma once

#include "Box.h"

struct AiContext {
  const Color color;
  ostream& log;
  double total_time{0.0};
};
