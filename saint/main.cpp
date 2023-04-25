#include "TimeStretchingRunner.h"
#include "rubberband/RubberBandStretcher.h"

#include <filesystem>
#include <iostream>
#include <utility>

namespace fs = std::filesystem;
using namespace RubberBand;

int main(int argc, const char *const *argv) {
  if (argc < 2) {
    std::cerr << "Expected at least 1 wav file argument." << std::endl;
    return 1;
  }
  const fs::path inputPath{std::string{argv[1]}};

  const std::vector<std::vector<RubberBandStretcher::Option>> optionsToCombine{
      // {RubberBandStretcher::OptionChannelsApart,
      //  RubberBandStretcher::OptionTransientsCrisp},
      // {RubberBandStretcher::OptionChannelsApart,
      //  RubberBandStretcher::OptionDetectorPercussive},
      // {RubberBandStretcher::OptionChannelsTogether,
      //  RubberBandStretcher::OptionTransientsCrisp},
      // {RubberBandStretcher::OptionChannelsTogether,
      //  RubberBandStretcher::OptionDetectorPercussive},
      // {RubberBandStretcher::OptionEngineFiner},
      {RubberBandStretcher::OptionWindowStandard}};

  for (const auto &ratio : std::vector<std::pair<int, int>>{{1, 2},
                                                            {2, 3},
                                                            {3, 4},
                                                            {4, 5},
                                                            {1, 1},
                                                            {5, 4},
                                                            {4, 3},
                                                            {3, 2},
                                                            {2, 1}}) {
    for (const auto &combination : optionsToCombine) {
      saint::TimeStretchingRunner::process(inputPath, combination, ratio);
    }
  }
}
