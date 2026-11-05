cmake_minimum_required(VERSION 3.16)
project(HillClimbRacingQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable compiler warnings for professional code quality
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find Qt packages (Supports both Qt 6 and Qt 5 seamlessly)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Multimedia Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Multimedia Network)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/network
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

set(BOX2D_VERSION 2.2.1)
set(BOX2D_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(BOX2D_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(BOX2D_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/Box2D EXCLUDE_FROM_ALL)

set(SOURCES
    src/main.cpp
    # Core Engine & State
    src/core/GameEngine.cpp
    src/core/GameStateManager.cpp
    src/core/InputController.cpp
    src/core/ProfileManager.cpp
    src/core/AccountManager.cpp
    src/core/LocalizationManager.cpp
    # Networking
    src/network/GameServer.cpp
    src/network/GameClient.cpp
    # Graphics & Software Rasterizer
    src/graphics/Framebuffer.cpp
    src/graphics/PixelCanvas.cpp
    src/graphics/RasterFont.cpp
    src/graphics/Sprite.cpp
    src/graphics/ScanlineRasterizer.cpp
    src/graphics/DayNightCycle.cpp
    src/graphics/EnvironmentRenderer.cpp
    src/graphics/Camera.cpp
    src/graphics/ParticleSystem.cpp
    # Physics Backend & Interfaces for teammates
    src/physics/SurfaceMaterial.cpp
    src/physics/TrackRecipe.cpp
    src/physics/TerrainConfig.cpp
    src/physics/Terrain.cpp
    src/physics/VehicleConfig.cpp
    src/physics/Vehicle.cpp
    src/physics/PhysicsWorld.cpp
    src/physics/Box2DPhysics.cpp
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
    src/ui/UIComponents.cpp
    src/ui/AccountScreen.cpp
    src/ui/LobbyScreen.cpp
    src/ui/ChallengeScreen.cpp
    src/ui/EvaluatorOverlay.cpp
    src/ui/MinimapRadar.cpp
    src/ui/DeveloperOverlay.cpp
    src/ui/UILayoutRegression.cpp
    # Audio
    src/audio/SoundManager.cpp
    # Vehicle & Driver Pixel Sprites
    src/graphics/VehicleSprites.cpp
    # Vehicle Concept Pixel Art Resources
    assets/vehicles.qrc
)

set(HEADERS
    src/core/GameEngine.h
    src/core/GameState.h
    src/core/GameStateManager.h
    src/core/InputController.h
    src/core/ProfileManager.h
    src/core/AccountManager.h
    src/core/LocalizationManager.h
    src/network/NetworkTypes.h
    src/network/GameServer.h
    src/network/GameClient.h
    src/graphics/Framebuffer.h
    src/graphics/PixelCanvas.h
    src/graphics/RasterFont.h
    src/graphics/Sprite.h
    src/graphics/VehicleSprites.h
    src/graphics/ScanlineRasterizer.h
    src/graphics/DayNightCycle.h
    src/graphics/EnvironmentRenderer.h
    src/graphics/Camera.h
    src/graphics/ParticleSystem.h
    src/physics/PhysicsTypes.h
    src/physics/SurfaceMaterial.h
    src/physics/TrackRecipe.h
    src/physics/TerrainConfig.h
    src/physics/Terrain.h
    src/physics/VehicleConfig.h
    src/physics/Vehicle.h
    src/physics/PhysicsWorld.h
    src/physics/Box2DPhysics.h
    src/ui/UITheme.h
    src/ui/UIComponents.h
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
    src/ui/AccountScreen.h
    src/ui/LobbyScreen.h
    src/ui/ChallengeScreen.h
    src/ui/EvaluatorOverlay.h
    src/ui/MinimapRadar.h
    src/ui/DeveloperOverlay.h
    src/ui/UILayoutRegression.h
    src/audio/SoundManager.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::Network
    Box2D
)
