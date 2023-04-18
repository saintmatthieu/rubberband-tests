add_library(wavfile
  WavFileReader.cpp
  WavFileWriter.cpp
  AudioContainer.cpp
)

target_link_libraries(wavfile PUBLIC 
  juce::juce_audio_utils
  juce::juce_audio_formats
)

target_include_directories(wavfile PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
