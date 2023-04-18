#pragma once

#include <filesystem>
#include <memory>

#include <juce_audio_utils/juce_audio_utils.h>

namespace saint {
class WavFileReader {
public:
  WavFileReader(const std::filesystem::path &path);
  int getNumSamplesPerChannelAvailable() const;
  int getSampleRate() const;
  int getNumChannels() const;
  void read(float *const *, int samplesPerChannel);

private:
  const std::unique_ptr<juce::AudioFormatReader> _juceReader;
  int _numReadSamples = 0;
};
} // namespace saint