#include "WavFileReader.h"

namespace fs = std::filesystem;

namespace saint {
namespace {
std::unique_ptr<juce::AudioFormatReader>
getJuceWavFileReader(const fs::path &path) {
  juce::WavAudioFormat format;
  std::unique_ptr<juce::AudioFormatReader> reader;
  reader.reset(format.createReaderFor(
      new juce::FileInputStream(juce::File(path.string())), true));
  return reader;
}
} // namespace

WavFileReader::WavFileReader(const fs::path &path)
    : _juceReader(getJuceWavFileReader(path)) {}

int WavFileReader::getNumSamplesPerChannelAvailable() const {
  return std::max(
      static_cast<int>(_juceReader->lengthInSamples) - _numReadSamples, 0);
}

int WavFileReader::getSampleRate() const {
  return static_cast<int>(_juceReader->sampleRate);
}

int WavFileReader::getNumChannels() const {
  return static_cast<int>(_juceReader->numChannels);
}

void WavFileReader::read(float *const *audio, int samplesPerChannel) {
  _juceReader->read(audio, _juceReader->numChannels,
                    static_cast<juce::int64>(_numReadSamples),
                    samplesPerChannel);
  _numReadSamples += samplesPerChannel;
}
} // namespace saint
