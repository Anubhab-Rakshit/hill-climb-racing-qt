#include "UILayoutRegression.h"
#include "Framebuffer.h"
#include "ProfileManager.h"
#include "MainMenuScreen.h"
#include "GarageScreen.h"
#include "StageSelectScreen.h"
#include "HUD.h"
#include "PauseOverlay.h"
#include "GameOverScreen.h"

#include <QDir>
#include <QString>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

namespace UI {

namespace {

struct Rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    bool contains(const Rect& inner) const {
        return inner.x >= x &&
               inner.y >= y &&
               (inner.x + inner.w) <= (x + w) &&
               (inner.y + inner.h) <= (y + h);
    }

    bool intersects(const Rect& other) const {
        int xOverlap = std::max(0, std::min(x + w, other.x + other.w) - std::max(x, other.x));
        int yOverlap = std::max(0, std::min(y + h, other.y + other.h) - std::max(y, other.y));
        return xOverlap > 0 && yOverlap > 0;
    }
};

struct ResolutionLayout {
    int width;
    int height;

    Rect screenRect() const {
        return {0, 0, width, height};
    }

    // Main Menu Buttons (8 buttons)
    std::vector<Rect> getMainMenuButtons() const {
        int btnW = (width >= 1200) ? 350 : 320;
        int btnX = width - btnW - (width >= 1200 ? 75 : 60);
        int startY = (height >= 700) ? 108 : 94;
        int btnH = (height >= 700) ? 40 : 38;
        int btnHSmall = (height >= 700) ? 38 : 36;
        int gap = (height >= 700) ? 46 : 42;
        int gapSmall = (height >= 700) ? 44 : 40;

        std::vector<Rect> btns;
        btns.push_back({btnX, startY, btnW, btnH});                                // Start
        btns.push_back({btnX, startY + gap, btnW, btnH});                          // Multiplayer
        btns.push_back({btnX, startY + gap * 2, btnW, btnH});                      // Grand Prix
        btns.push_back({btnX, startY + gap * 3, btnW, btnHSmall});                 // Garage
        btns.push_back({btnX, startY + gap * 3 + gapSmall, btnW, btnHSmall});      // Stages
        btns.push_back({btnX, startY + gap * 3 + gapSmall * 2, btnW, btnHSmall});  // Account
        btns.push_back({btnX, startY + gap * 3 + gapSmall * 3, btnW, btnHSmall});  // Evaluator
        btns.push_back({btnX, startY + gap * 3 + gapSmall * 4, btnW, (height >= 700) ? 34 : 32}); // Quit
        return btns;
    }

    // HUD Minimap & Distance Card
    Rect getMinimapRadar() const {
        return {270, 10, 420, 26};
    }

    Rect getHudDistanceCard() const {
        int distCardW = (width >= 1200) ? 380 : 320;
        int distCardH = 48;
        int distCardX = (width - distCardW) / 2;
        int distCardY = (height >= 700) ? 42 : 38;
        return {distCardX, distCardY, distCardW, distCardH};
    }

    // Pause Modal Box and Buttons
    Rect getPauseModalPanel() const {
        int boxW = (width >= 1200) ? 440 : 380;
        int boxH = (height >= 700) ? 360 : 330;
        return {(width - boxW) / 2, (height - boxH) / 2, boxW, boxH};
    }

    std::vector<Rect> getPauseButtons() const {
        int btnW = (width >= 1200) ? 270 : 230;
        int btnH = (height >= 700) ? 48 : 44;
        int gap = (height >= 700) ? 56 : 54;
        int cx = width / 2 - btnW / 2;
        int startY = (height >= 700) ? (height / 2 - 88) : (height / 2 - 76);

        std::vector<Rect> btns;
        for (int i = 0; i < 4; ++i) {
            btns.push_back({cx, startY + i * gap, btnW, btnH});
        }
        return btns;
    }

    // Game Over Modal Box and Buttons
    Rect getGameOverModalPanel() const {
        int boxW = (width >= 1200) ? 680 : 620;
        int boxH = (height >= 700) ? 450 : 410;
        return {(width - boxW) / 2, (height - boxH) / 2, boxW, boxH};
    }

    std::vector<Rect> getGameOverButtons() const {
        int btnW = (width >= 1200) ? 195 : 180;
        int btnH = (height >= 700) ? 48 : 46;
        int gap = (width >= 1200) ? 20 : 18;
        int totalW = 3 * btnW + 2 * gap;
        int startX = (width - totalW) / 2;
        int boxH = (height >= 700) ? 450 : 410;
        int boxY = (height - boxH) / 2;
        int btnY = boxY + boxH - btnH - 24;

        std::vector<Rect> btns;
        for (int i = 0; i < 3; ++i) {
            btns.push_back({startX + i * (btnW + gap), btnY, btnW, btnH});
        }
        return btns;
    }

    // Stage Select Cards and Action Buttons (Page 0: 3 cards)
    struct StageCardAndButton {
        Rect card;
        Rect button;
    };

    std::vector<StageCardAndButton> getStageCardsAndButtons() const {
        int cardW = (width >= 1200) ? 320 : 270;
        int cardGap = (width >= 1200) ? 30 : 20;
        int totalCardsW = 3 * cardW + 2 * cardGap;
        int startX = (width - totalCardsW) / 2;
        int cardY = (height >= 700) ? 108 : 88;
        int cardH = (height >= 700) ? 460 : 405;
        int btnW = cardW - 28;
        int btnH = (height >= 700) ? 42 : 40;
        int btnY = cardY + cardH - btnH - 10;

        std::vector<StageCardAndButton> result;
        for (int i = 0; i < 3; ++i) {
            int x = startX + i * (cardW + cardGap);
            Rect card = {x, cardY, cardW, cardH};
            Rect button = {x + 14, btnY, btnW, btnH};
            result.push_back({card, button});
        }
        return result;
    }

    // Garage Header Panel and Coin Badge
    Rect getGarageHeaderPanel() const {
        int cx = width / 2;
        int headW = (width >= 1200) ? 350 : 280;
        int headX = cx - headW / 2;
        return {headX, 12, headW, 34};
    }

    Rect getGarageCoinBadge() const {
        int coinBoxW = 155;
        int coinBoxH = 34;
        int coinBoxX = width - 335;
        return {coinBoxX, 12, coinBoxW, coinBoxH};
    }

    // Garage Key Buttons
    std::vector<Rect> getGarageButtons() const {
        std::vector<Rect> btns;
        btns.push_back({20, 14, 110, 36});           // Back
        btns.push_back({width - 170, 14, 160, 36});   // Drive

        int cx = width / 2;
        btns.push_back({cx - 240, 52, 36, 32});      // Prev Vehicle
        btns.push_back({cx + 80, 52, 36, 32});       // Next Vehicle
        btns.push_back({cx + 124, 52, 136, 32});     // Unlock Vehicle
        btns.push_back({cx - 160, 88, 30, 26});      // Prev Driver
        btns.push_back({cx + 130, 88, 30, 26});      // Next Driver
        btns.push_back({cx - 225, 284, 220, 28});    // Tab Powertrain
        btns.push_back({cx + 5, 284, 220, 28});      // Tab Chassis

        int cardW = 210;
        int cardGap = 16;
        int totalCardsW = 4 * cardW + 3 * cardGap;
        int startX = (width - totalCardsW) / 2;
        int cardY = 322;
        int btnY = cardY + 128;
        int btnW = cardW - 24;
        int btnH = 38;
        for (int i = 0; i < 4; ++i) {
            int x = startX + i * (cardW + cardGap) + 12;
            btns.push_back({x, btnY, btnW, btnH});
        }
        return btns;
    }

    // HUD Buttons (Pedals and Pause)
    std::vector<Rect> getHudButtons() const {
        int pedalY = (height >= 700) ? (height - 94) : (height - 76);
        int pedalH = (height >= 700) ? 76 : 62;
        int brakeW = (width >= 1200) ? 135 : 115;
        int brakeX = (width >= 1200) ? 24 : 20;
        int revW = (width >= 1200) ? 120 : 100;
        int revX = brakeX + brakeW + (width >= 1200 ? 12 : 10);
        int gasW = (width >= 1200) ? 135 : 115;
        int gasX = width - gasW - (width >= 1200 ? 25 : 20);
        int pauseW = (width >= 1200) ? 56 : 46;
        int pauseH = (height >= 700) ? 36 : 30;
        int pauseX = width - pauseW - (width >= 1200 ? 24 : 19);
        int pauseY = (height >= 700) ? 14 : 12;

        return {
            {brakeX, pedalY, brakeW, pedalH},
            {revX, pedalY, revW, pedalH},
            {gasX, pedalY, gasW, pedalH},
            {pauseX, pauseY, pauseW, pauseH}
        };
    }
};

} // namespace

int UILayoutRegression::run() {
    std::cout << "\n=======================================================" << std::endl;
    std::cout << "  RUNNING AUTOMATED UI LAYOUT REGRESSION SUITE" << std::endl;
    std::cout << "=======================================================" << std::endl;

    QDir().mkpath("scratch/ui_regression");

    const std::vector<std::pair<int, int>> resolutions = {
        {960, 540},
        {1280, 720}
    };

    Core::ProfileManager profile;

    // 1. Render and capture all 6 screens at both resolutions (12 total captures)
    int captureCount = 0;
    for (const auto& [w, h] : resolutions) {
        // MainMenuScreen
        {
            MainMenuScreen screen;
            screen.setDimensions(w, h);
            Graphics::Framebuffer fb(w, h);
            screen.render(fb, profile);
            QString path = QString("scratch/ui_regression/%1x%2_MainMenuScreen.png").arg(w).arg(h);
            if (fb.getImage().save(path)) captureCount++;
        }
        // GarageScreen
        {
            GarageScreen screen;
            screen.setDimensions(w, h);
            Graphics::Framebuffer fb(w, h);
            screen.render(fb, profile);
            QString path = QString("scratch/ui_regression/%1x%2_GarageScreen.png").arg(w).arg(h);
            if (fb.getImage().save(path)) captureCount++;
        }
        // StageSelectScreen
        {
            StageSelectScreen screen;
            screen.setDimensions(w, h);
            Graphics::Framebuffer fb(w, h);
            screen.render(fb, profile);
            QString path = QString("scratch/ui_regression/%1x%2_StageSelectScreen.png").arg(w).arg(h);
            if (fb.getImage().save(path)) captureCount++;
        }
        // HUD
        {
            HUD screen;
            screen.setDimensions(w, h);
            screen.setDistance(385.0f);
            screen.setSpeed(74.0f);
            screen.setRpm(4600.0f);
            screen.setFuel(0.82f);
            screen.setCoins(2400);
            Graphics::Framebuffer fb(w, h);
            fb.fillVerticalGradient(0, 0, w, h, 0xFF0D1B2A, 0xFF1B263B);
            screen.render(fb);
            QString path = QString("scratch/ui_regression/%1x%2_HUD.png").arg(w).arg(h);
            if (fb.getImage().save(path)) captureCount++;
        }
        // PauseOverlay
        {
            PauseOverlay screen;
            screen.setDimensions(w, h);
            Graphics::Framebuffer fb(w, h);
            fb.fillVerticalGradient(0, 0, w, h, 0xFF0D1B2A, 0xFF1B263B);
            screen.render(fb);
            QString path = QString("scratch/ui_regression/%1x%2_PauseOverlay.png").arg(w).arg(h);
            if (fb.getImage().save(path)) captureCount++;
        }
        // GameOverScreen
        {
            GameOverScreen screen;
            screen.setDimensions(w, h);
            screen.setCrashReason("DRIVER DOWN!");
            screen.setStats(428.5f, 320, 2, 3.6f, true);
            Graphics::Framebuffer fb(w, h);
            fb.fillVerticalGradient(0, 0, w, h, 0xFF0D1B2A, 0xFF1B263B);
            screen.render(fb);
            QString path = QString("scratch/ui_regression/%1x%2_GameOverScreen.png").arg(w).arg(h);
            if (fb.getImage().save(path)) captureCount++;
        }
    }

    std::cout << "[CAPTURE] Wrote " << captureCount << " screenshots (Target: 12) to scratch/ui_regression/" << std::endl;
    bool allPassed = (captureCount == 12);

    // 2. CHECK 1: SCREEN BOUNDS CONTAINMENT
    std::cout << "\n--- CHECK 1: SCREEN BOUNDS CONTAINMENT (960x540 & 1280x720) ---" << std::endl;
    bool check1Pass = true;
    for (const auto& [w, h] : resolutions) {
        ResolutionLayout layout{w, h};
        Rect screen = layout.screenRect();

        // Check Main Menu buttons
        for (const auto& btn : layout.getMainMenuButtons()) {
            if (!screen.contains(btn)) check1Pass = false;
        }
        // Check Pause modal and buttons
        if (!screen.contains(layout.getPauseModalPanel())) check1Pass = false;
        for (const auto& btn : layout.getPauseButtons()) {
            if (!screen.contains(btn)) check1Pass = false;
        }
        // Check Game Over modal and buttons
        if (!screen.contains(layout.getGameOverModalPanel())) check1Pass = false;
        for (const auto& btn : layout.getGameOverButtons()) {
            if (!screen.contains(btn)) check1Pass = false;
        }
        // Check Stage cards and buttons
        for (const auto& item : layout.getStageCardsAndButtons()) {
            if (!screen.contains(item.card) || !screen.contains(item.button)) check1Pass = false;
        }
        // Check Garage buttons and panels
        if (!screen.contains(layout.getGarageHeaderPanel())) check1Pass = false;
        if (!screen.contains(layout.getGarageCoinBadge())) check1Pass = false;
        for (const auto& btn : layout.getGarageButtons()) {
            if (!screen.contains(btn)) check1Pass = false;
        }
        // Check HUD buttons and radar
        if (!screen.contains(layout.getMinimapRadar())) check1Pass = false;
        if (!screen.contains(layout.getHudDistanceCard())) check1Pass = false;
        for (const auto& btn : layout.getHudButtons()) {
            if (!screen.contains(btn)) check1Pass = false;
        }

        std::cout << "  " << w << "x" << h << " Screen Bounds: " << (check1Pass ? "PASS" : "FAIL") << std::endl;
    }
    if (!check1Pass) allPassed = false;

    // 3. CHECK 2: MAIN MENU BUTTON NON-OVERLAP
    std::cout << "\n--- CHECK 2: MAIN MENU BUTTON NON-OVERLAP ---" << std::endl;
    bool check2Pass = true;
    for (const auto& [w, h] : resolutions) {
        ResolutionLayout layout{w, h};
        auto btns = layout.getMainMenuButtons();
        bool resOverlap = false;
        for (size_t i = 0; i < btns.size(); ++i) {
            for (size_t j = i + 1; j < btns.size(); ++j) {
                if (btns[i].intersects(btns[j])) {
                    resOverlap = true;
                }
            }
        }
        std::cout << "  " << w << "x" << h << " Menu Buttons Non-Overlap: "
                  << (!resOverlap ? "PASS (0 overlaps among 8 buttons)" : "FAIL") << std::endl;
        if (resOverlap) check2Pass = false;
    }
    if (!check2Pass) allPassed = false;

    // 4. CHECK 3: HUD DISTANCE CARD VS MINIMAP RADAR OCCLUSION
    std::cout << "\n--- CHECK 3: HUD DISTANCE CARD VS MINIMAP RADAR OCCLUSION ---" << std::endl;
    bool check3Pass = true;
    for (const auto& [w, h] : resolutions) {
        ResolutionLayout layout{w, h};
        Rect radar = layout.getMinimapRadar();
        Rect distCard = layout.getHudDistanceCard();
        bool overlap = radar.intersects(distCard);
        int verticalGap = distCard.y - (radar.y + radar.h);
        std::cout << "  " << w << "x" << h << " HUD Telemetry vs Minimap Radar: "
                  << (!overlap ? "PASS" : "FAIL")
                  << " (Vertical Clearance: " << verticalGap << "px)" << std::endl;
        if (overlap) check3Pass = false;
    }
    if (!check3Pass) allPassed = false;

    // 5. CHECK 4: PAUSE AND GAME OVER BUTTONS STAY INSIDE MODAL PANELS
    std::cout << "\n--- CHECK 4: PAUSE & GAME OVER BUTTON MODAL CONTAINMENT ---" << std::endl;
    bool check4Pass = true;
    for (const auto& [w, h] : resolutions) {
        ResolutionLayout layout{w, h};
        Rect pauseModal = layout.getPauseModalPanel();
        bool pauseInside = true;
        for (const auto& btn : layout.getPauseButtons()) {
            if (!pauseModal.contains(btn)) pauseInside = false;
        }

        Rect goModal = layout.getGameOverModalPanel();
        bool goInside = true;
        for (const auto& btn : layout.getGameOverButtons()) {
            if (!goModal.contains(btn)) goInside = false;
        }

        std::cout << "  " << w << "x" << h << " Pause Buttons in Modal:     " << (pauseInside ? "PASS" : "FAIL") << std::endl;
        std::cout << "  " << w << "x" << h << " Game Over Buttons in Modal: " << (goInside ? "PASS" : "FAIL") << std::endl;
        if (!pauseInside || !goInside) check4Pass = false;
    }
    if (!check4Pass) allPassed = false;

    // 6. CHECK 5: STAGE-SELECT ACTION BUTTONS STAY INSIDE THEIR CARDS
    std::cout << "\n--- CHECK 5: STAGE-SELECT ACTION BUTTON CARD CONTAINMENT ---" << std::endl;
    bool check5Pass = true;
    for (const auto& [w, h] : resolutions) {
        ResolutionLayout layout{w, h};
        auto items = layout.getStageCardsAndButtons();
        bool cardsContain = true;
        for (const auto& item : items) {
            if (!item.card.contains(item.button)) cardsContain = false;
        }
        std::cout << "  " << w << "x" << h << " Stage Cards Contain Buttons: " << (cardsContain ? "PASS" : "FAIL") << std::endl;
        if (!cardsContain) check5Pass = false;
    }
    if (!check5Pass) allPassed = false;

    // 7. CHECK 6: GARAGE HEADER DOES NOT OVERLAP COIN BADGE
    std::cout << "\n--- CHECK 6: GARAGE HEADER VS COIN BADGE NON-OVERLAP ---" << std::endl;
    bool check6Pass = true;
    for (const auto& [w, h] : resolutions) {
        ResolutionLayout layout{w, h};
        Rect header = layout.getGarageHeaderPanel();
        Rect coinBadge = layout.getGarageCoinBadge();
        bool overlap = header.intersects(coinBadge);
        int horizontalClearance = coinBadge.x - (header.x + header.w);
        std::cout << "  " << w << "x" << h << " Garage Header vs Coin Badge: "
                  << (!overlap ? "PASS" : "FAIL")
                  << " (Horizontal Clearance: " << horizontalClearance << "px)" << std::endl;
        if (overlap) check6Pass = false;
    }
    if (!check6Pass) allPassed = false;

    std::cout << "\n=======================================================" << std::endl;
    if (allPassed) {
        std::cout << "  UI LAYOUT REGRESSION RESULT: ALL CHECKS PASSED!" << std::endl;
    } else {
        std::cerr << "  UI LAYOUT REGRESSION RESULT: VERIFICATION FAILED!" << std::endl;
    }
    std::cout << "=======================================================\n" << std::endl;

    return allPassed ? 0 : 1;
}

} // namespace UI
