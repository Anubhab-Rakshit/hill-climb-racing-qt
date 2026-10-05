QT += core gui widgets multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = hill-climbing-qt
TEMPLATE = app

INCLUDEPATH += \
    src \
    src/core \
    src/graphics \
    src/physics \
    src/ui \
    src/audio

SOURCES += \
    src/main.cpp \
    src/core/GameEngine.cpp \
    src/core/GameStateManager.cpp \
    src/core/InputController.cpp \
    src/core/ProfileManager.cpp \
    src/graphics/Framebuffer.cpp \
    src/graphics/PixelCanvas.cpp \
    src/graphics/RasterFont.cpp \
    src/graphics/Sprite.cpp \
    src/graphics/ScanlineRasterizer.cpp \
    src/graphics/Camera.cpp \
    src/graphics/ParticleSystem.cpp \
    src/physics/Terrain.cpp \
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
    src/audio/SoundManager.cpp

HEADERS += \
    src/core/GameEngine.h \
    src/core/GameState.h \
    src/core/GameStateManager.h \
    src/core/InputController.h \
    src/core/ProfileManager.h \
    src/graphics/Framebuffer.h \
    src/graphics/PixelCanvas.h \
    src/graphics/RasterFont.h \
    src/graphics/Sprite.h \
    src/graphics/ScanlineRasterizer.h \
    src/graphics/Camera.h \
    src/graphics/ParticleSystem.h \
    src/physics/PhysicsTypes.h \
    src/physics/Terrain.h \
    src/physics/Vehicle.h \
    src/physics/PhysicsWorld.h \
    src/ui/UITheme.h \
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
    src/audio/SoundManager.h
