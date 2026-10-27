cmake_minimum_required(VERSION 3.14)

# Add GoogleTest via FetchContent
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# Prevent overriding parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Build test executable
add_executable(ItemRendererTests
    ItemRendererTest.cpp
)

# Link against the game library components needed (ItemRenderer, Framebuffer, Camera, Terrain)
# Assuming these are part of the main executable target, we link the source files directly for simplicity
target_link_libraries(ItemRendererTests PRIVATE
    GTest::gtest_main
    ${PROJECT_NAME}   # link the main executable which includes needed objects
)

include(GoogleTest)
gtest_discover_tests(ItemRendererTests)
