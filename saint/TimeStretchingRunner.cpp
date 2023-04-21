#include "TimeStretchingRunner.h"
#include "AudioContainer.h"
#include "WavFileReader.h"
#include "WavFileWriter.h"

#include <cassert>
#include <memory>
#include <optional>

namespace saint {
using namespace RubberBand;
using namespace std::literals::string_literals;
namespace fs = std::filesystem;

namespace {
RubberBandStretcher::Option
mergeOptions(const std::vector<RubberBandStretcher::Option> &options) {
  auto merged =
      static_cast<int>(RubberBandStretcher::Option::OptionProcessRealTime);
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
getOptionSuffix(const std::vector<RubberBandStretcher::Option> &options) {
  std::string optionSuffix;
  std::for_each(options.begin(), options.end(),
                [&](RubberBandStretcher::Option option) {
                  if (const auto nonDefaultString = asString(option)) {
                    optionSuffix += "-"s + *nonDefaultString;
                  }
                });
  return optionSuffix;
}
} // namespace

void process(const fs::path &inputPath, const fs::path &outputPath,
             RubberBandStretcher::Option options) {

  WavFileReader wavReader{inputPath};
  const auto numChannels = wavReader.getNumChannels();
  WavFileWriter wavWriter{outputPath, numChannels, wavReader.getSampleRate()};
  RubberBandStretcher stretcher{static_cast<size_t>(wavReader.getSampleRate()),
                                static_cast<size_t>(numChannels), options, 2.0};

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

namespace {
void writeWav(const fs::path &path, int sampleRate, float *const *audio,
              int numSamples, int numChannels) {
  WavFileWriter leftWriter{path, numChannels, sampleRate};
  leftWriter.write(audio, numSamples);
}
} // namespace

void TimeStretchingRunner::process(
    const std::filesystem::path &inputPath,
    const std::vector<RubberBand::RubberBandStretcher::Option> &options) {

  WavFileReader reader{inputPath};
  assert(reader.getNumChannels() == 2);
  if (reader.getNumChannels() != 2) {
    std::cerr << "Not interested in mono input" << std::endl;
    return;
  }
  const auto sampleRate = reader.getSampleRate();
  const auto numSamples = reader.getNumSamplesPerChannelAvailable();
  AudioContainer inContainer{numSamples, 2};
  reader.read(inContainer.get(), numSamples);
  const auto dir = inputPath.parent_path();
  const auto tmpLeftInPath = dir / "_tmp_left_in.wav";
  const auto tmpRightInPath = dir / "_tmp_right_in.wav";
  const auto tmpLeftOutPath = dir / "_tmp_left_out.wav";
  const auto tmpRightOutPath = dir / "_tmp_right_out.wav";
  writeWav(tmpLeftInPath, sampleRate, inContainer.get(), numSamples, 1);
  writeWav(tmpRightInPath, sampleRate, &inContainer.get()[1], numSamples, 1);

  const auto optionSuffix = getOptionSuffix(options);
  const auto mergedOptions = mergeOptions(options);
  const auto outputPathHead =
      inputPath.parent_path() /
      inputPath.stem().concat("-out").concat(optionSuffix);
  const auto outputPathStereo = fs::path{outputPathHead}.concat("-st.wav");
  const auto outputPathDualMono = fs::path{outputPathHead}.concat("-dm.wav");

  saint::process(inputPath, outputPathStereo, mergedOptions);
  saint::process(tmpLeftInPath, tmpLeftOutPath, mergedOptions);
  saint::process(tmpRightInPath, tmpRightOutPath, mergedOptions);

  WavFileReader leftOutReader{tmpLeftOutPath};
  WavFileReader rightOutReader{tmpRightOutPath};
  const auto numLeftSamples = leftOutReader.getNumSamplesPerChannelAvailable();
  const auto numRightSamples =
      rightOutReader.getNumSamplesPerChannelAvailable();
  //  This assert sometimes fails => the same number of input samples doesn't
  //  yield the same number of output samples, even for files with as similar
  //  contents as the left-right channels of a stereo file.
  // assert(numLeftSamples == numRightSamples);
  const auto outNumSamples = std::max(numLeftSamples, numRightSamples);
  AudioContainer outContainer{outNumSamples, 2};
  leftOutReader.read(outContainer.get(), outNumSamples);
  rightOutReader.read(&outContainer.get()[1], outNumSamples);
  writeWav(outputPathDualMono, sampleRate, outContainer.get(), outNumSamples,
           2);
}

} // namespace saint