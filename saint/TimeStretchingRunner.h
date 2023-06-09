#pragma once

#include "rubberband/RubberBandStretcher.h"

#include <filesystem>
#include <utility>
#include <vector>

namespace saint {
struct TimeStretchingRunner {
  static void
  process(const std::filesystem::path &inputPath,
          const std::vector<RubberBand::RubberBandStretcher::Option> &options,
          const std::pair<int, int> &stretchRatio);
};
} // namespace saint
