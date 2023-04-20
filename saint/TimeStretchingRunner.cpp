#include "TimeStretchingRunner.h"
#include "AudioContainer.h"
#include "WavFileReader.h"
#include "WavFileWriter.h"

#include <cassert>
#include <optional>

namespace saint {
using namespace RubberBand;
using namespace std::literals::string_literals;
namespace fs = std::filesystem;

namespace {
RubberBandStretcher::Option
mergeOptions(const std::vector<RubberBandStretcher::Option> &options) {
  auto merged = 0;
  for (const auto option : options) {
    merged |= option;
  }
  return static_cast<RubberBandStretcher::Option>(merged);
}

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

std::string
getFileSuffix(const std::vector<RubberBandStretcher::Option> &options) {
  std::string fileSuffix;
  std::for_each(options.begin(), options.end(),
                [&](RubberBandStretcher::Option option) {
                  if (const auto nonDefaultString = asString(option)) {
                    fileSuffix += "-"s + *nonDefaultString;
                  }
                });
  return fileSuffix;
}

} // namespace

void TimeStretchingRunner::process(
    const fs::path &inputPath,
    const std::vector<RubberBandStretcher::Option> &options) {

  const auto fileSuffix = getFileSuffix(options);
  const fs::path outputPath =
      inputPath.parent_path() /
      inputPath.stem().concat("-out").concat(fileSuffix).concat(".wav");
  const auto mergedOptions = mergeOptions(options);
  WavFileReader wavReader{inputPath};
  const auto numChannels = wavReader.getNumChannels();
  WavFileWriter wavWriter{outputPath, numChannels, wavReader.getSampleRate()};
  RubberBandStretcher stretcher{static_cast<size_t>(wavReader.getSampleRate()),
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
    AudioContainer readContainer(static_cast<int>(numSamplesToRead),
                                 static_cast<int>(numChannels));
    finalCall = numRequired >= numAvailable;
    wavReader.read(readContainer.get(), static_cast<int>(numSamplesToRead));
    stretcher.process(readContainer.get(), numSamplesToRead, finalCall);
    if (stretcher.available() == -1) {
      std::cout << "Really ?.." << std::endl;
      break;
    }
    const auto numSamplesToWrite = static_cast<size_t>(stretcher.available());
    AudioContainer writeContainer(static_cast<int>(numSamplesToWrite),
                                  static_cast<int>(numChannels));
    const auto retrieved =
        stretcher.retrieve(writeContainer.get(), numSamplesToWrite);
    assert(retrieved == numSamplesToWrite);
    const auto numSamplesToTrim =
        std::min(numSamplesToWrite, remainingLeadDelay);
    remainingLeadDelay -= numSamplesToTrim;
    if (numSamplesToTrim == numSamplesToWrite) {
      continue;
    }
    wavWriter.write(
        writeContainer.getWithOffset(static_cast<int>(numSamplesToTrim)),
        static_cast<int>(numSamplesToWrite - numSamplesToTrim));
  }
  // Squeeze out the final few samples
  while (stretcher.available() > 0) {
    const auto numSamplesToWrite = static_cast<size_t>(stretcher.available());
    AudioContainer writeContainer(stretcher.available(), numChannels);
    const auto retrieved =
        stretcher.retrieve(writeContainer.get(), numSamplesToWrite);
    assert(retrieved == numSamplesToWrite);
    wavWriter.write(writeContainer.get(), static_cast<int>(numSamplesToWrite));
  }
}

} // namespace saint