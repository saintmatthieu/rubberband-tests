#pragma once

#include "rubberband/RubberBandStretcher.h"

#include <filesystem>
#include <vector>

namespace saint {
struct TimeStretchingRunner {
  static void
  process(const std::filesystem::path &inputPath,
          const std::vector<RubberBand::RubberBandStretcher::Option> &options);
};
} // namespace saint
