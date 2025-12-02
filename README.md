# core

Core primitives for use in embedded OSUSat firmware

Documentation can be found [here](https://osusat.github.io/core/)

# Provided Primitives

The current primitives available are:

- Ring buffer (arbitrary size, using modulo)
- Ring buffer Power of 2 (capacity must be a power of 2, but runs faster using a bitmask)
- Event bus

# Project Integration

- Add this repository as a submodule of your project:
    - `git submodule add https://github.com/osusat/core.git lib/`
- Link the C library using CMake
    - Add the library as a subdirectory using `add_subdirectory(lib/core ${CMAKE_CURRENT_BINARY_DIR}/osusat-core-build)`
    - Link the library using `target_link_libraries(my_c_project PRIVATE OSUSat::Core)`
