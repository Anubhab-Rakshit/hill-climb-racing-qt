QMAKE_PROJECT_DEPTH = 0
QT += core gui widgets multimedia network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
DEFINES += _USE_MATH_DEFINES

win32-msvc* {
    QMAKE_CXXFLAGS_DEBUG -= -Zi
    QMAKE_CXXFLAGS_DEBUG += -Z7
    QMAKE_CFLAGS_DEBUG -= -Zi
    QMAKE_CFLAGS_DEBUG += -Z7
}

TARGET = hill-climbing-qt
TEMPLATE = app

INCLUDEPATH += \
    src \
    src/core \
    src/graphics \
    src/physics \
    src/ui \
    src/audio \
    src/network

SOURCES += \
    src/main.cpp \
    src/core/GameEngine.cpp \
    src/core/GameStateManager.cpp \
    src/core/InputController.cpp \
    src/core/ProfileManager.cpp \
    src/core/AccountManager.cpp \
    src/core/LocalizationManager.cpp \
    src/network/GameServer.cpp \
    src/network/GameClient.cpp \
    src/graphics/Framebuffer.cpp \
    src/graphics/PixelCanvas.cpp \
    src/graphics/RasterFont.cpp \
    src/graphics/Sprite.cpp \
    src/graphics/ScanlineRasterizer.cpp \
    src/graphics/DayNightCycle.cpp \
    src/graphics/EnvironmentRenderer.cpp \
    src/graphics/Camera.cpp \
    src/graphics/ParticleSystem.cpp \
    src/graphics/VehicleSprites.cpp \
    src/physics/TerrainConfig.cpp \
    src/physics/Terrain.cpp \
    src/physics/VehicleConfig.cpp \
    src/physics/Vehicle.cpp \
    src/physics/PhysicsWorld.cpp \
    src/ui/RetroButton.cpp \
    src/ui/AnalogGauge.cpp \
    src/ui/FuelBar.cpp \
    src/ui/StuntBanner.cpp \
    src/ui/HUD.cpp \
    src/ui/MainMenuScreen.cpp \
    src/ui/GarageScreen.cpp \
    src/ui/StageSelectScreen.cpp \
    src/ui/PauseOverlay.cpp \
    src/ui/GameOverScreen.cpp \
    src/ui/UIComponents.cpp \
    src/ui/AccountScreen.cpp \
    src/ui/LobbyScreen.cpp \
    src/ui/ChallengeScreen.cpp \
    src/ui/EvaluatorOverlay.cpp \
    src/ui/MinimapRadar.cpp \
    src/ui/DeveloperOverlay.cpp \
    src/audio/SoundManager.cpp

HEADERS += \
    src/core/GameEngine.h \
    src/core/GameState.h \
    src/core/GameStateManager.h \
    src/core/InputController.h \
    src/core/ProfileManager.h \
    src/core/AccountManager.h \
    src/core/LocalizationManager.h \
    src/network/NetworkTypes.h \
    src/network/GameServer.h \
    src/network/GameClient.h \
    src/graphics/Framebuffer.h \
    src/graphics/PixelCanvas.h \
    src/graphics/RasterFont.h \
    src/graphics/Sprite.h \
    src/graphics/VehicleSprites.h \
    src/graphics/CarSpritesData.h \
    src/graphics/ScanlineRasterizer.h \
    src/graphics/DayNightCycle.h \
    src/graphics/EnvironmentRenderer.h \
    src/graphics/Camera.h \
    src/graphics/ParticleSystem.h \
    src/physics/PhysicsTypes.h \
    src/physics/TerrainConfig.h \
    src/physics/Terrain.h \
    src/physics/VehicleConfig.h \
    src/physics/Vehicle.h \
    src/physics/PhysicsWorld.h \
    src/ui/UITheme.h \
    src/ui/UIComponents.h \
    src/ui/RetroButton.h \
    src/ui/AnalogGauge.h \
    src/ui/FuelBar.h \
    src/ui/StuntBanner.h \
    src/ui/HUD.h \
    src/ui/MainMenuScreen.h \
    src/ui/GarageScreen.h \
    src/ui/StageSelectScreen.h \
    src/ui/PauseOverlay.h \
    src/ui/GameOverScreen.h \
    src/ui/AccountScreen.h \
    src/ui/LobbyScreen.h \
    src/ui/ChallengeScreen.h \
    src/ui/EvaluatorOverlay.h \
    src/ui/MinimapRadar.h \
    src/ui/DeveloperOverlay.h \
    src/audio/SoundManager.h

RESOURCES += \
    assets/vehicles.qrc
