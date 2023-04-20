#include "AudioContainer.h"
#include "WavFileReader.h"
#include "WavFileWriter.h"
#include "rubberband/RubberBandStretcher.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>

namespace fs = std::filesystem;
using namespace RubberBand;
using namespace saint;
using namespace std::literals::string_literals;

std::optional<std::string> asString(RubberBandStretcher::Option option) {
  switch (option) {
  case RubberBandStretcher::OptionProcessRealTime:
    return "ProcessRealTime";
  case RubberBandStretcher::OptionStretchPrecise:
    return "StretchPrecise";
  case RubberBandStretcher::OptionTransientsMixed:
    return "TransientsMixed";
  case RubberBandStretcher::OptionTransientsSmooth:
    return "TransientsSmooth";
  case RubberBandStretcher::OptionDetectorPercussive:
    return "DetectorPercussive";
  case RubberBandStretcher::OptionDetectorSoft:
    return "DetectorSoft";
  case RubberBandStretcher::OptionPhaseIndependent:
    return "PhaseIndependent";
  case RubberBandStretcher::OptionThreadingNever:
    return "ThreadingNever";
  case RubberBandStretcher::OptionThreadingAlways:
    return "ThreadingAlways";
  case RubberBandStretcher::OptionWindowShort:
    return "WindowShort";
  case RubberBandStretcher::OptionWindowLong:
    return "WindowLong";
  case RubberBandStretcher::OptionSmoothingOn:
    return "SmoothingOn";
  case RubberBandStretcher::OptionFormantPreserved:
    return "FormantPreserved";
  case RubberBandStretcher::OptionPitchHighQuality:
    return "PitchHighQuality";
  case RubberBandStretcher::OptionPitchHighConsistency:
    return "PitchHighConsistency";
  case RubberBandStretcher::OptionChannelsTogether:
    return "ChannelsTogether";
  case RubberBandStretcher::OptionEngineFiner:
    return "EngineFiner";
  default:
    return std::nullopt;
  }
}

RubberBandStretcher::Option
mergeOptions(const std::vector<RubberBandStretcher::Option> &options) {
  auto merged = 0;
  for (const auto option : options) {
    merged |= option;
  }
  return static_cast<RubberBandStretcher::Option>(merged);
}

std::string
getFileSuffice(const std::vector<RubberBandStretcher::Option> &options) {
  std::string fileSuffix;
  std::for_each(options.begin(), options.end(),
                [&](RubberBandStretcher::Option option) {
                  if (const auto nonDefaultString = asString(option)) {
                    fileSuffix += "-"s + *nonDefaultString;
                  }
                });
  return fileSuffix;
}

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
    const auto fileSuffix = getFileSuffice(combination);
    const fs::path outputPath =
        inputPath.parent_path() /
        inputPath.stem().concat("-out").concat(fileSuffix).concat(".wav");
    const auto mergedOptions = mergeOptions(combination);
    WavFileReader wavReader{inputPath};
    const auto numChannels = wavReader.getNumChannels();
    WavFileWriter wavWriter{outputPath, numChannels, wavReader.getSampleRate()};
    RubberBandStretcher stretcher{
        static_cast<size_t>(wavReader.getSampleRate()),
        static_cast<size_t>(numChannels),
        RubberBandStretcher::OptionProcessRealTime |
            RubberBandStretcher::OptionChannelsTogether,
        2.0};

    auto finalCall = false;

    {
      const auto startPad = stretcher.getPreferredStartPad();
      AudioContainer container(static_cast<int>(startPad),
                               static_cast<int>(numChannels));
      stretcher.process(container.get(), startPad, finalCall);
    }

    auto remainingLeadDelay = stretcher.getStartDelay();
    while (!finalCall) {
      const auto numRequired = stretcher.getSamplesRequired();
      const auto numAvailable =
          static_cast<size_t>(wavReader.getNumSamplesPerChannelAvailable());
      const auto numSamplesToRead = std::min(numAvailable, numRequired);
      AudioContainer readContainer(numSamplesToRead,
                                   static_cast<int>(numChannels));
      finalCall = numRequired >= numAvailable;
      wavReader.read(readContainer.get(), static_cast<int>(numSamplesToRead));
      stretcher.process(readContainer.get(), numSamplesToRead, finalCall);
      if (stretcher.available() == -1) {
        std::cout << "Really ?.." << std::endl;
        break;
      }
      const auto numSamplesToWrite = static_cast<size_t>(stretcher.available());
      AudioContainer writeContainer(numSamplesToWrite, numChannels);
      const auto retrieved =
          stretcher.retrieve(writeContainer.get(), numSamplesToWrite);
      assert(retrieved == numSamplesToWrite);
      const auto numSamplesToTrim =
          std::min(numSamplesToWrite, remainingLeadDelay);
      remainingLeadDelay -= numSamplesToTrim;
      if (numSamplesToTrim == numSamplesToWrite) {
        continue;
      }
      wavWriter.write(writeContainer.getWithOffset(numSamplesToTrim),
                      numSamplesToWrite - numSamplesToTrim);
    }
    // Squeeze out the final few samples
    while (stretcher.available() > 0) {
      const auto numSamplesToWrite = static_cast<size_t>(stretcher.available());
      AudioContainer writeContainer(stretcher.available(), numChannels);
      const auto retrieved =
          stretcher.retrieve(writeContainer.get(), numSamplesToWrite);
      assert(retrieved == numSamplesToWrite);
      wavWriter.write(writeContainer.get(), numSamplesToWrite);
    }
  }
}
