add_executable(rubberband-tests
  main.cpp
)

target_link_libraries(rubberband-tests
  PRIVATE
    rubberband-cmake
)