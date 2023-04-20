#include "TimeStretchingRunner.h"
#include "rubberband/RubberBandStretcher.h"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
using namespace RubberBand;

int main(int argc, const char *const *argv) {
  if (argc < 2) {
    std::cerr << "Expected at least 1 wav file argument." << std::endl;
    return 1;
  }
  const fs::path inputPath{std::string{argv[1]}};

  const std::vector<std::vector<RubberBandStretcher::Option>> optionsToCombine{
      {RubberBandStretcher::OptionChannelsApart,
       RubberBandStretcher::OptionEngineFaster},
      {RubberBandStretcher::OptionChannelsApart,
       RubberBandStretcher::OptionEngineFiner},
      {RubberBandStretcher::OptionChannelsTogether,
       RubberBandStretcher::OptionEngineFaster},
      {RubberBandStretcher::OptionChannelsTogether,
       RubberBandStretcher::OptionEngineFiner}};

  for (const auto &combination : optionsToCombine) {
    saint::TimeStretchingRunner::process(inputPath, combination);
  }
}
