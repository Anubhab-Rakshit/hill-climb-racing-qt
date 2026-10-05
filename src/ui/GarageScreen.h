#pragma once

#include "Framebuffer.h"
#include "RetroButton.h"
#include "ProfileManager.h"
#include <functional>

namespace UI {

/**
 * @brief Interactive Vehicle Garage & Tuning Shop.
 * Allows upgrading Engine, Suspension, Tires, and 4WD with live vehicle preview.
 */
class GarageScreen {
public:
    GarageScreen();

    void setDimensions(int width, int height);

    void setOnBack(std::function<void()> cb) { m_btnBack.setOnClick(cb); }
    void setOnDrive(std::function<void()> cb) { m_btnDrive.setOnClick(cb); }

    void onMouseMove(int px, int py);
    bool onMouseDown(int px, int py, Core::ProfileManager& profile);
    void onMouseUp(int px, int py);

    void update(float dt);
    void render(Graphics::Framebuffer& fb, Core::ProfileManager& profile);

private:
    int m_width;
    int m_height;
    float m_carBounceY;
    float m_carBounceVel;

    RetroButton m_btnBack;
    RetroButton m_btnDrive;

    RetroButton m_btnUpgradeEngine;
    RetroButton m_btnUpgradeSuspension;
    RetroButton m_btnUpgradeTires;
    RetroButton m_btnUpgrade4WD;

    void drawVehiclePreview(Graphics::Framebuffer& fb, int cx, int cy, const Core::ProfileManager& profile);
};

} // namespace UI
