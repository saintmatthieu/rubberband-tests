#include "WavFileWriter.h"

namespace saint {
namespace fs = std::filesystem;

namespace {
std::unique_ptr<juce::AudioFormatWriter>
getJuceWavFileWriter(const fs::path &path, int numChannels, int sampleRate) {
  juce::WavAudioFormat format;
  std::unique_ptr<juce::AudioFormatWriter> writer;
  if (fs::exists(path)) {
    fs::remove(path);
  }
  writer.reset(format.createWriterFor(
      new juce::FileOutputStream(juce::File(path.string())),
      static_cast<double>(sampleRate), numChannels, 16, {}, 0));
  return writer;
}
} // namespace

WavFileWriter::WavFileWriter(const fs::path &path, int numChannels,
                             int sampleRate)
    : _juceWriter(getJuceWavFileWriter(path, numChannels, sampleRate)),
      _numChannels(numChannels) {}

bool WavFileWriter::write(const float *const *audio, int samplesPerChannel) {
  return _juceWriter->writeFromFloatArrays(audio, _numChannels,
                                           samplesPerChannel);
}
} // namespace saint
