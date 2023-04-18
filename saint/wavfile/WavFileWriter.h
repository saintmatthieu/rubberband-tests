#pragma once

#include <filesystem>
#include <memory>

#include <juce_audio_utils/juce_audio_utils.h>

namespace saint {
class WavFileWriter {
public:
  WavFileWriter(const std::filesystem::path &path, int numChannels,
                int sampleRate);
  bool write(const float *const *, int samplesPerChannel);

private:
  const std::unique_ptr<juce::AudioFormatWriter> _juceWriter;
  const int _numChannels;
};
} // namespace saint