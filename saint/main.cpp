#include "AudioContainer.h"
#include "WavFileReader.h"
#include "WavFileWriter.h"
#include "rubberband/RubberBandStretcher.h"


#include <cassert>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
using namespace RubberBand;
using namespace saint;

int main(int argc, const char *const *argv) {
  if (argc < 2) {
    std::cerr << "Expected at least 1 wav file argument." << std::endl;
    return 1;
  }
  const fs::path inputPath{std::string{argv[1]}};

  const fs::path outputPath = inputPath.parent_path().append("output.wav");
  saint::WavFileReader wavReader{inputPath};
  const auto numChannels = wavReader.getNumChannels();
  saint::WavFileWriter wavWriter{outputPath, numChannels,
                                 wavReader.getSampleRate()};
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
