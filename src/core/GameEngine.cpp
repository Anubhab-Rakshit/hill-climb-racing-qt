#include "GameEngine.h"
#include "UITheme.h"
#include <algorithm>

namespace Core {

GameEngine::GameEngine(QWidget* parent)
    : QMainWindow(parent)
    , m_framebuffer(UI::Theme::VIRTUAL_WIDTH, UI::Theme::VIRTUAL_HEIGHT)
    , m_canvas(new Graphics::PixelCanvas(m_framebuffer, m_input, this))
    , m_accumulator(0.0f)
{
    setWindowTitle("Hill Climb Racing (Qt C++ Pure-Raster Edition)");
    resize(UI::Theme::VIRTUAL_WIDTH, UI::Theme::VIRTUAL_HEIGHT); // 1:1 Pixel-perfect native window size (zero scaling cost)
    setCentralWidget(m_canvas);

    m_input.setOnKeyTyped([this](int key, const std::string& text) {
        m_stateManager.onKeyTyped(key, text);
    });

    connect(&m_tickTimer, &QTimer::timeout, this, &GameEngine::onTick);
}

GameEngine::~GameEngine() {
    m_tickTimer.stop();
}

void GameEngine::start() {
    m_tickTimer.setTimerType(Qt::PreciseTimer);
    m_elapsedTimer.start();
    // 60 FPS display tick interval (~16.6 ms)
    m_tickTimer.start(16);
}

void GameEngine::onTick() {
    float realDt = m_elapsedTimer.restart() / 1000.0f;
    // Hard cap realDt to at most 100ms (0.10s) to prevent runaway accumulators
    if (realDt > 0.10f) realDt = 0.10f;

    m_accumulator += realDt;

    // Process input once per display frame (avoids clearing transient mouse clicks during multi-substep loops)
    m_stateManager.handleInput(m_input.state());
    m_input.resetTransientActions();

    // Fixed timestep physics tick at 120 Hz, strictly bounded to prevent expensive catch-up spirals
    const float FIXED_DT = 1.0f / 120.0f;
    const int MAX_SUBSTEPS = 3; // Never execute more than 3 physics ticks (~25ms) in a single frame
    int subSteps = 0;

    while (m_accumulator >= FIXED_DT && subSteps < MAX_SUBSTEPS) {
        m_stateManager.fixedUpdate(FIXED_DT);
        m_accumulator -= FIXED_DT;
        subSteps++;
    }

    // If still behind due to a system hitch or window move, clamp accumulator so lag never persists
    if (m_accumulator >= FIXED_DT) {
        m_accumulator = 0.0f;
    }

    // Render state with interpolation
    float alpha = m_accumulator / FIXED_DT;
    m_stateManager.render(m_framebuffer, alpha);

    // Request Qt Repaint
    m_canvas->update();
}

} // namespace Core
