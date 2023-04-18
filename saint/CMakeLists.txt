add_subdirectory(wavfile)

add_executable(rubberband-tests
  main.cpp
)

target_link_libraries(rubberband-tests
  PRIVATE
    wavfile
    rubberband-cmake
)