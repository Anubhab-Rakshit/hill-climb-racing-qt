#pragma once

#include "Framebuffer.h"
#include "InputController.h"

namespace Core {

class GameEngine;

/**
 * @brief Abstract Base Class for Game States (State Pattern).
 */
class IGameState {
public:
    virtual ~IGameState() = default;

    virtual void enter() = 0;
    virtual void exit() = 0;
    virtual void handleInput(const InputController::State& input) = 0;
    virtual void fixedUpdate(float dt) = 0;
    virtual void render(Graphics::Framebuffer& fb, float alpha) = 0;
};

} // namespace Core
