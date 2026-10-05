cmake_minimum_required(VERSION 3.16)
project(HillClimbRacingQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable compiler warnings for professional code quality
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find Qt packages (Supports both Qt 6 and Qt 5 seamlessly)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Multimedia)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio
)

set(SOURCES
    src/main.cpp
    # Core Engine & State
    src/core/GameEngine.cpp
    src/core/GameStateManager.cpp
    src/core/InputController.cpp
    src/core/ProfileManager.cpp
    # Graphics & Software Rasterizer
    src/graphics/Framebuffer.cpp
    src/graphics/PixelCanvas.cpp
    src/graphics/RasterFont.cpp
    src/graphics/Sprite.cpp
    src/graphics/ScanlineRasterizer.cpp
    src/graphics/Camera.cpp
    src/graphics/ParticleSystem.cpp
    # Physics Backend & Interfaces for teammates
    src/physics/Terrain.cpp
    src/physics/Vehicle.cpp
    src/physics/PhysicsWorld.cpp
    # UI / UX Layer
    src/ui/RetroButton.cpp
    src/ui/AnalogGauge.cpp
    src/ui/FuelBar.cpp
    src/ui/StuntBanner.cpp
    src/ui/HUD.cpp
    src/ui/MainMenuScreen.cpp
    src/ui/GarageScreen.cpp
    src/ui/StageSelectScreen.cpp
    src/ui/PauseOverlay.cpp
    src/ui/GameOverScreen.cpp
    # Audio
    src/audio/SoundManager.cpp
)

set(HEADERS
    src/core/GameEngine.h
    src/core/GameState.h
    src/core/GameStateManager.h
    src/core/InputController.h
    src/core/ProfileManager.h
    src/graphics/Framebuffer.h
    src/graphics/PixelCanvas.h
    src/graphics/RasterFont.h
    src/graphics/Sprite.h
    src/graphics/ScanlineRasterizer.h
    src/graphics/Camera.h
    src/graphics/ParticleSystem.h
    src/physics/PhysicsTypes.h
    src/physics/Terrain.h
    src/physics/Vehicle.h
    src/physics/PhysicsWorld.h
    src/ui/UITheme.h
    src/ui/RetroButton.h
    src/ui/AnalogGauge.h
    src/ui/FuelBar.h
    src/ui/StuntBanner.h
    src/ui/HUD.h
    src/ui/MainMenuScreen.h
    src/ui/GarageScreen.h
    src/ui/StageSelectScreen.h
    src/ui/PauseOverlay.h
    src/ui/GameOverScreen.h
    src/audio/SoundManager.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
)
